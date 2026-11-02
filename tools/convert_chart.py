#!/usr/bin/env python3
"""
Convierte un chart JSON de Psych Engine / FNF en un chart_data.h para el DS.

Uso:
    python convert_chart.py chart.json include/chart_data.h
    python convert_chart.py chart.json include/chart_data.h --player-only

Cada nota del JSON es [strumTime, noteData, sustainLength, noteType].
  column  = noteData % 4
  mustHit = (noteData < 4) == mustHitSection
"""
import argparse
import json
import sys

MAX_NOTES = 2000  # el mismo valor que MAX_NOTES en chart.h


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    ap.add_argument("output")
    ap.add_argument("--player-only", action="store_true",
                    help="solo exporta las notas del jugador (mustHit = 1)")
    args = ap.parse_args()

    with open(args.input, encoding="utf-8") as f:
        data = json.load(f)
    # Psych viejo: data["song"] es un dict. Psych v1: "song" es el nombre (str)
    # y todo cuelga de la raiz.
    song = data.get("song")
    if not isinstance(song, dict):
        song = data

    bpm = song.get("bpm")
    if bpm is None and song.get("notes"):
        bpm = song["notes"][0].get("bpm")   # fallback: bpm de la primera seccion
    if bpm is None:
        print("AVISO: no encuentro bpm, uso 120", file=sys.stderr)
        bpm = 120
    bpm = float(bpm)
    speed = float(song.get("speed", 1))

    # tipo de nota -> numero. 0 siempre es "normal".
    type_ids = {"": 0}

    def type_id(raw):
        # puede venir como string, como int (charts antiguos) o no venir
        if raw is None or raw == 0 or raw is False:
            key = ""
        else:
            key = str(raw)
        if key not in type_ids:
            type_ids[key] = len(type_ids)
        return type_ids[key]

    notes = []
    bpm_changes = 0
    for sec in song.get("notes", []):
        if sec.get("changeBPM"):
            bpm_changes += 1
        must_hit_section = bool(sec.get("mustHitSection", True))
        for n in sec.get("sectionNotes", []):
            if len(n) < 2:
                continue
            time = int(round(float(n[0])))
            note_data = int(n[1])
            if note_data < 0:  # eventos, no son notas
                continue
            sustain = int(round(float(n[2]))) if len(n) > 2 and n[2] else 0
            raw_type = n[3] if len(n) > 3 else None

            column = note_data % 4
            must_hit = 1 if ((note_data < 4) == must_hit_section) else 0
            if args.player_only and not must_hit:
                continue
            notes.append((time, sustain, column, type_id(raw_type), must_hit))

    notes.sort(key=lambda x: x[0])

    # ---- avisos ----
    if bpm_changes:
        print(f"AVISO: el chart tiene {bpm_changes} cambios de BPM; "
              f"solo se exporta el BPM inicial ({bpm}).", file=sys.stderr)
    if len(notes) > MAX_NOTES:
        print(f"AVISO: {len(notes)} notas > MAX_NOTES ({MAX_NOTES}). "
              f"Sube MAX_NOTES en chart.h.", file=sys.stderr)

    # ---- escribir el .h ----
    with open(args.output, "w", encoding="utf-8") as out:
        out.write("// Generado por convert_chart.py. No editar a mano.\n")
        out.write("#ifndef CHART_DATA_H\n#define CHART_DATA_H\n")
        out.write("#include <stdint.h>\n\n")
        out.write(f"#define CHART_BPM {bpm}f\n")
        out.write(f"#define CHART_SPEED {speed}f\n")
        out.write(f"#define CHART_NOTE_COUNT {len(notes)}\n\n")
        out.write("// tipos de nota: " +
                  ", ".join(f"{v}={k or 'normal'}" for k, v in type_ids.items()) + "\n\n")
        out.write("typedef struct {\n"
                  "    int strumTime;\n"
                  "    int sustainLength;\n"
                  "    uint8_t column;\n"
                  "    uint8_t type;\n"
                  "    uint8_t mustHit;\n"
                  "} ChartNoteData;\n\n")
        out.write("static const ChartNoteData chartData[CHART_NOTE_COUNT] = {\n")
        for t, s, c, ty, m in notes:
            out.write(f"    {{{t}, {s}, {c}, {ty}, {m}}},\n")
        out.write("};\n\n#endif\n")

    n_player = sum(1 for n in notes if n[4])
    print(f"OK: {len(notes)} notas ({n_player} del jugador), bpm {bpm}, speed {speed}")
    print("Tipos de nota:", {(k or 'normal'): v for k, v in type_ids.items()})


if __name__ == "__main__":
    main()
