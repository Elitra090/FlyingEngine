#!/usr/bin/env python3
"""Mezcla uno o varios audios, los pasa a mono, remuestrea y codifica IMA-ADPCM
continuo para el hardware de sonido de la DS (cabecera de 4 bytes).

Uso:  python ogg2dsadpcm.py salida.bin [--rate 16000] in1.ogg in2.ogg ...
Requiere ffmpeg y numpy.
"""
import argparse, subprocess, sys
import numpy as np

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,
        88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
        544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
        2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
        10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]


def decode_pcm(path, rate):
    out = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", str(rate),
         "-f", "f32le", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(out, dtype="<f4")


def encode(pcm16):
    n = len(pcm16)
    s = pcm16.tolist()
    pred, idx = s[0], 0
    nib = []
    ap = nib.append
    for i in range(1, n):
        step = STEP[idx]
        diff = s[i] - pred
        code = 0
        if diff < 0:
            code = 8
            diff = -diff
        d = step >> 3
        if diff >= step:
            code |= 4; diff -= step; d += step
        step >>= 1
        if diff >= step:
            code |= 2; diff -= step; d += step
        step >>= 1
        if diff >= step:
            code |= 1; d += step
        pred += -d if code & 8 else d
        if pred > 32767: pred = 32767
        elif pred < -32768: pred = -32768
        idx += IDX[code & 7]
        if idx < 0: idx = 0
        elif idx > 88: idx = 88
        ap(code)
    while (len(nib) + 8) % 8:        # datos multiplo de 4 bytes (8 nibbles)
        ap(0)
    data = bytearray()
    head = (s[0] & 0xFFFF) | (0 << 16)  # indice de paso inicial = 0
    data += head.to_bytes(4, "little")
    for i in range(0, len(nib), 2):
        data.append(nib[i] | (nib[i + 1] << 4))   # nibble bajo primero
    return bytes(data)


def decode(data):
    pred = int.from_bytes(data[0:2], "little", signed=True)
    idx = data[2]
    out = [pred]
    for b in data[4:]:
        for code in (b & 15, b >> 4):
            step = STEP[idx]
            d = step >> 3
            if code & 4: d += step
            if code & 2: d += step >> 1
            if code & 1: d += step >> 2
            pred += -d if code & 8 else d
            pred = max(-32768, min(32767, pred))
            idx = max(0, min(88, idx + IDX[code & 7]))
            out.append(pred)
    return np.array(out, dtype=np.int16)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("output")
    ap.add_argument("inputs", nargs="+")
    ap.add_argument("--rate", type=int, default=16000)
    a = ap.parse_args()

    tracks = [decode_pcm(p, a.rate) for p in a.inputs]
    n = max(len(t) for t in tracks)
    mix = np.zeros(n, dtype=np.float32)
    for t in tracks:
        mix[:len(t)] += t
    peak = float(np.max(np.abs(mix)))
    if peak > 0.98:
        print(f"Pico {peak:.2f} > 1: normalizo (x{0.98/peak:.2f})", file=sys.stderr)
        mix *= 0.98 / peak
    pcm = np.clip(mix * 32767.0, -32768, 32767).astype(np.int16)

    data = encode(pcm)
    with open(a.output, "wb") as f:
        f.write(data)

    # verificacion: decodifica y compara con el original
    dec = decode(data)
    m = min(len(dec), len(pcm))
    err = pcm[:m].astype(np.float64) - dec[:m]
    snr = 10 * np.log10(np.sum(pcm[:m].astype(np.float64) ** 2) / max(np.sum(err ** 2), 1))
    print(f"OK: {len(data)} bytes ({len(data)/1048576:.2f} MB), {a.rate} Hz mono, "
          f"{len(pcm)/a.rate:.1f} s, SNR {snr:.1f} dB")
    print(f"En C: frecuencia = {a.rate}, tamano = {len(data)}")


if __name__ == "__main__":
    main()
