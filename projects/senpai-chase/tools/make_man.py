# Builds the "chibi" sprite of the man from the cutout photo:
# real head and torso from the photo + legs drawn and animated (walk cycle).
# Output: list of RGBA PIL frames (walk right x8, idle x2) and mirrored ones (left)
import math
from PIL import Image, ImageDraw

S = 4                    # supersampling
W, H = 44, 74            # final sprite size
N_WALK = 8

def build(cut_path):
    cut = Image.open(cut_path).convert("RGBA")
    head = cut.crop((470, 0, 960, 610))
    torso = cut.crop((380, 470, 960, 1010))
    hw = 31 * S
    head = head.resize((hw, int(hw * head.height / head.width)), Image.LANCZOS)
    tw = 25 * S
    torso = torso.resize((tw, int(tw * torso.height / torso.width)), Image.LANCZOS)
    return head, torso

SKIN = (236, 196, 172, 255)
SHORTS = (28, 34, 60, 255)
CROCS = (238, 238, 232, 255)
OUT = (25, 20, 30, 255)

def leg(d, hipx, hipy, ang, knee):
    """Draws a leg (thigh + calf + croc) at 4x scale."""
    L1, L2 = 8 * S, 7 * S
    kx = hipx + L1 * math.sin(ang); ky = hipy + L1 * math.cos(ang)
    a2 = ang - knee
    fx = kx + L2 * math.sin(a2); fy = ky + L2 * math.cos(a2)
    w = 5 * S
    d.line([(hipx, hipy), (kx, ky)], fill=OUT, width=w + 2 * S)
    d.line([(kx, ky), (fx, fy)], fill=OUT, width=w + S)
    d.line([(hipx, hipy), (kx, ky)], fill=SKIN, width=w)
    d.line([(kx, ky), (fx, fy)], fill=SKIN, width=w - S)
    # shorts on the thigh
    mx = hipx + 0.55 * (kx - hipx); my = hipy + 0.55 * (ky - hipy)
    d.line([(hipx, hipy - 2 * S), (mx, my)], fill=SHORTS, width=w + 2 * S)
    # croc
    d.ellipse([fx - 3.2 * S, fy - 2 * S, fx + 4.5 * S, fy + 2.6 * S], fill=OUT)
    d.ellipse([fx - 2.6 * S, fy - 1.4 * S, fx + 3.9 * S, fy + 2.0 * S], fill=CROCS)

def frame(head, torso, phase, walking, blink=False):
    im = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    bob = abs(math.sin(phase)) * 1.6 * S if walking else (math.sin(phase) * 0.5 + 0.5) * 0.8 * S
    cx = W * S // 2
    hipy = (H - 18) * S - bob
    sw = math.sin(phase) * 0.55 if walking else 0
    kn = (max(0, math.sin(phase + 1.6)) * 0.9) if walking else 0
    kn2 = (max(0, math.sin(phase + 1.6 + math.pi)) * 0.9) if walking else 0
    # back leg first, then torso, then front leg (slightly darker in back)
    leg(d, cx - 3 * S, hipy, -sw, kn2)
    # shorts (pelvis)
    d.rounded_rectangle([cx - 8 * S, hipy - 5 * S, cx + 8 * S, hipy + 3 * S], 3 * S, fill=SHORTS, outline=OUT, width=S)
    leg(d, cx + 3 * S, hipy, sw, kn)
    # torso from the photo
    ty = int(hipy - torso.height + 3 * S)
    im.alpha_composite(torso, (cx - torso.width // 2 + 2 * S, ty))
    # head from the photo (slight tilt while walking)
    h = head.rotate(math.sin(phase) * 4 if walking else 0, resample=Image.BICUBIC, expand=False)
    hy = int(ty - head.height + 12 * S)
    im.alpha_composite(h, (cx - head.width // 2 + 3 * S, hy))
    im = im.resize((W, H), Image.LANCZOS)
    # dark outline around the alpha mask for readability on the background
    a = im.getchannel("A").point(lambda v: 255 if v > 110 else 0)
    from PIL import ImageFilter
    grow = a.filter(ImageFilter.MaxFilter(3))
    outl = Image.new("RGBA", im.size, (20, 16, 24, 255)); outl.putalpha(grow)
    outl.alpha_composite(Image.composite(im, Image.new("RGBA", im.size, (0, 0, 0, 0)), a))
    return outl

def all_frames(cut_path):
    head, torso = build(cut_path)
    right = [frame(head, torso, 2 * math.pi * i / N_WALK, True) for i in range(N_WALK)]
    right += [frame(head, torso, 0, False), frame(head, torso, math.pi / 2, False)]
    left = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in right]
    return right, left

if __name__ == "__main__":
    import sys
    r, l = all_frames(sys.argv[1])
    sheet = Image.new("RGBA", (W * len(r), H * 2), (90, 160, 90, 255))
    for i, f in enumerate(r): sheet.alpha_composite(f, (i * W, 0))
    for i, f in enumerate(l): sheet.alpha_composite(f, (i * W, H))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(sys.argv[2])
