import sys
import os
import json
import struct
import re

def parse_chart_data(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    notes = []

    try:
        data = json.loads(content)
        song_info = data.get("song", data) if isinstance(data, dict) else data

        if isinstance(song_info, dict) and "notes" in song_info:
            for section in song_info["notes"]:
                must_hit_sec = section.get("mustHitSection", True)
                for n in section.get("sectionNotes", []):
                    if len(n) >= 2:
                        strum = int(round(float(n[0])))
                        raw_col = int(n[1])
                        sus = int(round(float(n[2]))) if len(n) > 2 and n[2] is not None else 0

                        n_type = 0
                        if len(n) > 3 and n[3] is not None:
                            if isinstance(n[3], int):
                                n_type = n[3]
                            elif isinstance(n[3], str) and n[3].isdigit():
                                n_type = int(n[3])

                        col = raw_col % 4
                        must_hit = 1 if ((raw_col < 4 and must_hit_sec) or (raw_col >= 4 and not must_hit_sec)) else 0

                        # Guardar ÚNICAMENTE las notas del jugador (4 flechas)
                        if must_hit == 1:
                            notes.append((strum, sus, col, n_type, must_hit))
            return notes

        if isinstance(data, list):
            for item in data:
                if isinstance(item, dict):
                    must_hit = int(item.get("mustHit", 1))
                    if must_hit == 1:
                        notes.append((
                            int(round(float(item.get("strumTime", 0)))),
                            int(round(float(item.get("sustainLength", 0)))),
                            int(item.get("column", 0)),
                            int(item.get("type", 0)),
                            must_hit
                        ))
            return notes

    except json.JSONDecodeError:
        pass

    # Soporte para archivos .h tradicionales (monochrome.h)
    pattern = re.compile(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}')
    for m in pattern.findall(content):
        must_hit = int(m[4])
        if must_hit == 1:
            notes.append((int(m[0]), int(m[1]), int(m[2]), int(m[3]), must_hit))

    return notes

def main():
    if len(sys.argv) < 2:
        print("Uso: python3 convertidor.py <archivo.json|archivo.h> [salida.chartbin]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) >= 3 else os.path.splitext(input_path)[0] + ".chartbin"

    notes = parse_chart_data(input_path)

    if not notes:
        print("Error: No se encontraron notas del jugador.")
        sys.exit(1)

    # Ordenar por tiempo para evitar bloqueos en C
    notes.sort(key=lambda x: x[0])

    with open(output_path, 'wb') as f:
        # 1. Cabecera ORIGINAL: Solo 4 bytes con la cantidad total de notas (int32)
        f.write(struct.pack('<i', len(notes)))

        # 2. Notas: 12 bytes por nota (strum, sus, col, type, mustHit + padding)
        fmt = '<i i B B B x'
        for strum, sus, col, n_type, must_hit in notes:
            f.write(struct.pack(fmt, strum, sus, col, n_type, must_hit))

    print(f"Convertido con éxito: '{input_path}' -> '{output_path}' ({len(notes)} notas del jugador)")

if __name__ == '__main__':
    main()