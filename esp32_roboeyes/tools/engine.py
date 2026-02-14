#!/usr/bin/env python3
"""
RoboEyes Animation Engine v1.0
================================
Gera frames de animação a partir de YAML com suporte a:
- Easing (linear, ease_in, ease_out, ease_in_out)
- Primitivas (circle, rect, line, text, ellipse)
- Canvas clipping automático
- Inversão global de cores
- Exportação PNG + GIF preview

Uso:
    python engine.py animations/example.yaml
    python engine.py animations/example.yaml --output frames/
    python engine.py animations/example.yaml --preview
"""

import argparse
import math
import os
import sys

try:
    import yaml
except ImportError:
    print("❌ Instale PyYAML: pip install pyyaml")
    sys.exit(1)

try:
    from PIL import Image, ImageDraw, ImageFont, ImageOps
except ImportError:
    print("❌ Instale Pillow: pip install pillow")
    sys.exit(1)


# ============================================================
# EASING FUNCTIONS
# ============================================================

def linear(t):
    """Movimento constante, sem suavização."""
    return t

def ease_in(t):
    """Começa devagar, acelera no final (quadrático)."""
    return t * t

def ease_out(t):
    """Começa rápido, desacelera no final (quadrático)."""
    return t * (2.0 - t)

def ease_in_out(t):
    """Suave no início e no final (quadrático)."""
    if t < 0.5:
        return 2.0 * t * t
    return -1.0 + (4.0 - 2.0 * t) * t

def ease_in_cubic(t):
    """Começa devagar, acelera forte (cúbico)."""
    return t * t * t

def ease_out_cubic(t):
    """Começa rápido, para suave (cúbico)."""
    t -= 1.0
    return t * t * t + 1.0

def ease_in_out_cubic(t):
    """Suave dos dois lados (cúbico)."""
    if t < 0.5:
        return 4.0 * t * t * t
    t -= 1.0
    return 0.5 * (4.0 * t * t * t + 2.0)

def ease_bounce(t):
    """Efeito de quicar."""
    if t < 1.0 / 2.75:
        return 7.5625 * t * t
    elif t < 2.0 / 2.75:
        t -= 1.5 / 2.75
        return 7.5625 * t * t + 0.75
    elif t < 2.5 / 2.75:
        t -= 2.25 / 2.75
        return 7.5625 * t * t + 0.9375
    else:
        t -= 2.625 / 2.75
        return 7.5625 * t * t + 0.984375

def ease_elastic(t):
    """Efeito elástico."""
    if t == 0 or t == 1:
        return t
    return -(2 ** (10 * (t - 1))) * math.sin((t - 1.075) * (2 * math.pi) / 0.3)


EASING_MAP = {
    "linear":          linear,
    "ease_in":         ease_in,
    "ease_out":        ease_out,
    "ease_in_out":     ease_in_out,
    "ease_in_cubic":   ease_in_cubic,
    "ease_out_cubic":  ease_out_cubic,
    "ease_in_out_cubic": ease_in_out_cubic,
    "ease_bounce":     ease_bounce,
    "ease_elastic":    ease_elastic,
}


# ============================================================
# INTERPOLATION
# ============================================================

def interpolate(val_a, val_b, t_norm, easing_fn):
    """Interpola entre dois valores (int ou float) usando easing."""
    eased = easing_fn(t_norm)
    return val_a + (val_b - val_a) * eased


def get_keyframe_values(keyframes, time_norm, easing_fn, properties):
    """
    Dado uma lista de keyframes e o tempo normalizado (0..1),
    encontra os dois keyframes adjacentes e interpola as propriedades.
    """
    # Ordenar keyframes por t
    kfs = sorted(keyframes, key=lambda k: k["t"])

    # Antes do primeiro keyframe
    if time_norm <= kfs[0]["t"]:
        return {p: kfs[0].get(p, 0) for p in properties}

    # Depois do último keyframe
    if time_norm >= kfs[-1]["t"]:
        return {p: kfs[-1].get(p, 0) for p in properties}

    # Encontrar par de keyframes
    for i in range(len(kfs) - 1):
        if kfs[i]["t"] <= time_norm <= kfs[i + 1]["t"]:
            t_local = (time_norm - kfs[i]["t"]) / (kfs[i + 1]["t"] - kfs[i]["t"])
            result = {}
            for p in properties:
                a = kfs[i].get(p, 0)
                b = kfs[i + 1].get(p, 0)
                result[p] = interpolate(a, b, t_local, easing_fn)
            return result

    return {p: kfs[-1].get(p, 0) for p in properties}


# ============================================================
# CANVAS CLIPPING
# ============================================================

def clamp(val, min_val, max_val):
    """Garante que o valor está dentro do range [min, max]."""
    return max(min_val, min(val, max_val))


def clip_rect(x, y, w, h, canvas_w, canvas_h):
    """Retorna retângulo clipado. None se totalmente fora."""
    x1 = clamp(int(x), 0, canvas_w - 1)
    y1 = clamp(int(y), 0, canvas_h - 1)
    x2 = clamp(int(x + w), 0, canvas_w)
    y2 = clamp(int(y + h), 0, canvas_h)
    if x2 <= x1 or y2 <= y1:
        return None
    return (x1, y1, x2, y2)


def clip_circle(cx, cy, r, canvas_w, canvas_h):
    """Retorna bounding box clipado do círculo. None se fora."""
    x1 = int(cx - r)
    y1 = int(cy - r)
    x2 = int(cx + r)
    y2 = int(cy + r)
    if x2 < 0 or y2 < 0 or x1 >= canvas_w or y1 >= canvas_h:
        return None
    return (
        clamp(x1, 0, canvas_w - 1),
        clamp(y1, 0, canvas_h - 1),
        clamp(x2, 0, canvas_w),
        clamp(y2, 0, canvas_h),
    )


# ============================================================
# DRAWING PRIMITIVES
# ============================================================

def draw_object(draw, obj_type, vals, canvas_w, canvas_h, fill=1):
    """Desenha um objeto no canvas com clipping seguro."""

    if obj_type == "circle":
        cx = vals.get("x", 0)
        cy = vals.get("y", 0)
        r = vals.get("r", 5)
        bbox = clip_circle(cx, cy, r, canvas_w, canvas_h)
        if bbox is None:
            return
        draw.ellipse(
            [int(cx - r), int(cy - r), int(cx + r), int(cy + r)],
            fill=fill, outline=fill
        )

    elif obj_type == "rect":
        x = vals.get("x", 0)
        y = vals.get("y", 0)
        w = vals.get("w", 10)
        h = vals.get("h", 10)
        r = vals.get("r", 0)  # border radius
        box = clip_rect(x, y, w, h, canvas_w, canvas_h)
        if box is None:
            return
        if r > 0:
            draw.rounded_rectangle(
                [int(x), int(y), int(x + w), int(y + h)],
                radius=int(r), fill=fill, outline=fill
            )
        else:
            draw.rectangle(
                [int(x), int(y), int(x + w), int(y + h)],
                fill=fill, outline=fill
            )

    elif obj_type == "ellipse":
        cx = vals.get("x", 0)
        cy = vals.get("y", 0)
        rx = vals.get("rx", 10)
        ry = vals.get("ry", 5)
        draw.ellipse(
            [int(cx - rx), int(cy - ry), int(cx + rx), int(cy + ry)],
            fill=fill, outline=fill
        )

    elif obj_type == "line":
        x1 = clamp(int(vals.get("x1", 0)), 0, canvas_w - 1)
        y1 = clamp(int(vals.get("y1", 0)), 0, canvas_h - 1)
        x2 = clamp(int(vals.get("x2", 10)), 0, canvas_w - 1)
        y2 = clamp(int(vals.get("y2", 10)), 0, canvas_h - 1)
        width = int(vals.get("stroke", 1))
        draw.line([(x1, y1), (x2, y2)], fill=fill, width=width)

    elif obj_type == "text":
        x = clamp(int(vals.get("x", 0)), 0, canvas_w - 1)
        y = clamp(int(vals.get("y", 0)), 0, canvas_h - 1)
        text = vals.get("text", "")
        size = int(vals.get("size", 10))
        try:
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", size)
        except (IOError, OSError):
            font = ImageFont.load_default()
        draw.text((x, y), str(text), fill=fill, font=font)

    elif obj_type == "pixel":
        x = int(vals.get("x", 0))
        y = int(vals.get("y", 0))
        if 0 <= x < canvas_w and 0 <= y < canvas_h:
            draw.point((x, y), fill=fill)


# ============================================================
# PROPERTIES MAP PER OBJECT TYPE
# ============================================================

OBJECT_PROPERTIES = {
    "circle":  ["x", "y", "r"],
    "rect":    ["x", "y", "w", "h", "r"],
    "ellipse": ["x", "y", "rx", "ry"],
    "line":    ["x1", "y1", "x2", "y2", "stroke"],
    "text":    ["x", "y", "size"],
    "pixel":   ["x", "y"],
}


# ============================================================
# FRAME GENERATOR
# ============================================================

def generate_frames(config):
    """Gera todos os frames de todas as animações definidas no YAML."""

    canvas_w = config.get("canvas", {}).get("width", 128)
    canvas_h = config.get("canvas", {}).get("height", 64)
    fps = config.get("fps", 12)
    invert = config.get("invert_global", False)
    bg_color = 0  # preto

    all_frames = {}

    for anim in config.get("animations", []):
        name = anim.get("name", "untitled")
        duration = anim.get("duration", 1.0)
        total_frames = max(1, int(duration * fps))
        frames = []

        print(f"  🎬 {name}: {total_frames} frames ({duration}s @ {fps}fps)")

        for frame_idx in range(total_frames):
            t_norm = frame_idx / max(1, total_frames - 1)

            # Criar frame 1-bit
            img = Image.new("1", (canvas_w, canvas_h), bg_color)
            draw = ImageDraw.Draw(img)

            # Desenhar cada objeto
            for obj in anim.get("objects", []):
                obj_type = obj.get("type", "circle")
                easing_name = obj.get("easing", "linear")
                easing_fn = EASING_MAP.get(easing_name, linear)
                keyframes = obj.get("keyframes", [])
                props = OBJECT_PROPERTIES.get(obj_type, ["x", "y"])

                # Adicionar propriedades estáticas (como text)
                static_vals = {}
                if "text" in obj:
                    static_vals["text"] = obj["text"]

                vals = get_keyframe_values(keyframes, t_norm, easing_fn, props)
                vals.update(static_vals)

                fill = obj.get("fill", 1)
                draw_object(draw, obj_type, vals, canvas_w, canvas_h, fill)

            # Inversão global
            if invert:
                img = ImageOps.invert(img.convert("L")).convert("1")

            frames.append(img)

        all_frames[name] = frames

    return all_frames, canvas_w, canvas_h, fps


# ============================================================
# OUTPUT
# ============================================================

def save_frames(all_frames, output_dir):
    """Salva frames como PNGs organizados por animação."""
    os.makedirs(output_dir, exist_ok=True)
    total = 0

    for name, frames in all_frames.items():
        anim_dir = os.path.join(output_dir, name)
        os.makedirs(anim_dir, exist_ok=True)

        for i, frame in enumerate(frames):
            path = os.path.join(anim_dir, f"frame_{i:04d}.png")
            frame.save(path)
            total += 1

    return total


def save_preview_gif(all_frames, output_dir, fps):
    """Gera GIF preview de cada animação."""
    for name, frames in all_frames.items():
        if len(frames) < 2:
            continue
        gif_path = os.path.join(output_dir, f"{name}_preview.gif")
        duration_ms = max(1, int(1000 / fps))

        # Converter 1-bit para P mode para GIF
        gif_frames = []
        for f in frames:
            p = f.convert("P")
            gif_frames.append(p)

        gif_frames[0].save(
            gif_path,
            save_all=True,
            append_images=gif_frames[1:],
            duration=duration_ms,
            loop=0,
        )
        print(f"  📹 Preview: {gif_path}")


# ============================================================
# MAIN
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="🤖 RoboEyes Animation Engine — Gera frames OLED a partir de YAML"
    )
    parser.add_argument("config", help="Arquivo YAML de configuração")
    parser.add_argument("--output", "-o", default="output",
                        help="Diretório de saída (padrão: output)")
    parser.add_argument("--preview", "-p", action="store_true",
                        help="Gerar GIF preview")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Modo verboso")
    args = parser.parse_args()

    if not os.path.exists(args.config):
        print(f"❌ Arquivo não encontrado: {args.config}")
        sys.exit(1)

    # Parse YAML
    with open(args.config, "r") as f:
        config = yaml.safe_load(f)

    canvas = config.get("canvas", {})
    print(f"\n{'='*50}")
    print(f"🤖 RoboEyes Animation Engine v1.0")
    print(f"{'='*50}")
    print(f"📐 Canvas: {canvas.get('width', 128)}x{canvas.get('height', 64)}")
    print(f"🎞️  FPS: {config.get('fps', 12)}")
    print(f"🔄 Inversão: {'SIM' if config.get('invert_global', False) else 'NÃO'}")
    print(f"💾 Limite memória: {config.get('memory_limit', 32)} KB")
    print(f"{'='*50}\n")

    # Gerar frames
    print("🔧 Gerando frames...")
    all_frames, canvas_w, canvas_h, fps = generate_frames(config)

    # Salvar
    total = save_frames(all_frames, args.output)
    print(f"\n✅ {total} frames salvos em '{args.output}/'")

    # Preview GIF
    if args.preview:
        save_preview_gif(all_frames, args.output, fps)

    # Calcular tamanho estimado
    bytes_per_frame = (canvas_w * canvas_h) // 8
    total_bytes = total * bytes_per_frame
    memory_limit = config.get("memory_limit", 32) * 1024

    print(f"\n📊 Estatísticas:")
    print(f"   Frames totais: {total}")
    print(f"   Bytes/frame: {bytes_per_frame}")
    print(f"   Total: {total_bytes:,} bytes ({total_bytes/1024:.1f} KB)")

    if total_bytes > memory_limit:
        limit_kb = memory_limit // 1024
        print(f"\n   ⚠️  \033[93mWARNING: Animação excede {limit_kb}KB! "
              f"({total_bytes/1024:.1f}KB > {limit_kb}KB)\033[0m")
        print(f"   💡 Reduza FPS, duração, ou resolução.")
    else:
        used_pct = (total_bytes / memory_limit) * 100
        print(f"   ✅ \033[92mDentro do limite ({used_pct:.0f}% de "
              f"{memory_limit//1024}KB)\033[0m")

    print(f"\n{'='*50}")
    print(f"✨ Próximo passo: python c_array.py {args.output}/ "
          f"--config {args.config}")
    print(f"{'='*50}\n")


if __name__ == "__main__":
    main()
