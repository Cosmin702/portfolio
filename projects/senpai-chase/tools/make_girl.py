# 3D anime girl modelled with SDF (signed distance fields), rendered by ray marching
# in cel-shading (toon) with outlines. Pre-renders 8 directions x (8 walk frames + 2 idle).
import math
import numpy as np
from PIL import Image, ImageFilter

SS = 3                       # supersampling
W, H = 60, 82                # final sprite size
PPU = 25.5                   # pixels per 3D unit
PITCH = math.radians(35)     # camera tilted from above
FOOT_Y = H - 11              # sprite row where the feet sit
N_WALK = 8

# ---------------------------------------------------------------- SDF helpers
def length(v): return np.sqrt((v * v).sum(-1))
def dot(a, b): return (a * b).sum(-1)

def sd_capsule(p, a, b, r1, r2=None):
    if r2 is None: r2 = r1
    a = np.asarray(a, float); b = np.asarray(b, float)
    pa = p - a; ba = b - a
    h = np.clip(dot(pa, ba) / dot(ba, ba), 0, 1)
    return length(pa - ba * h[:, None]) - (r1 + (r2 - r1) * h)

def sd_ellipsoid(p, c, r):
    q = (p - np.asarray(c, float)) / np.asarray(r, float)
    k0 = length(q); k1 = length(q / np.asarray(r, float))
    return k0 * (k0 - 1) / np.maximum(k1, 1e-6)

def sd_box(p, c, b):
    q = np.abs(p - np.asarray(c, float)) - np.asarray(b, float)
    return length(np.maximum(q, 0)) + np.minimum(q.max(-1), 0)

def smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b * (1 - h) + a * h - k * h * (1 - h)

def rot_x(v, a):
    v = np.asarray(v, float); c, s = math.cos(a), math.sin(a)
    return np.array([v[0], c * v[1] - s * v[2], s * v[1] + c * v[2]])

# ---------------------------------------------------------------- materials
SKIN, HAIR, SHIRT, SKIRT, LEG, SHOE, RIBBON = range(7)
PAL = {
    SKIN: (255, 222, 205), HAIR: (250, 140, 195), SHIRT: (248, 248, 255),
    SKIRT: (52, 62, 140), LEG: (40, 34, 52), SHOE: (120, 52, 48), RIBBON: (230, 40, 70),
}

class Pose:
    """Joint positions of the character for a given animation phase."""
    def __init__(self, phase, walking, blink=False):
        self.blink = blink
        sw = 0.55 * math.sin(phase) if walking else 0.0
        bob = 0.04 * math.cos(2 * phase) if walking else 0.015 * math.sin(phase)
        self.lean = 0.08 if walking else 0.0
        y0 = 0.02 + bob
        self.H = np.array([0, 1.80 + y0, 0.03 if walking else 0])
        self.hip_y = 0.76 + y0
        self.legs = []
        for side, a, kphase in ((-1, sw, phase), (1, -sw, phase + math.pi)):
            knee_bend = (0.75 * max(0.0, math.sin(kphase + math.pi / 2)) + 0.1) if walking else 0.05
            hip = np.array([0.105 * side, self.hip_y, 0])
            d1 = np.array([0, -math.cos(a), math.sin(a)])
            knee = hip + 0.34 * d1
            a2 = a - knee_bend
            d2 = np.array([0, -math.cos(a2), math.sin(a2)])
            ankle = knee + 0.33 * d2
            self.legs.append((hip, knee, ankle))
        self.arms = []
        for side, a in ((-1, -sw * 0.8), (1, sw * 0.8)):
            sh = np.array([0.215 * side, 1.18 + y0, 0])
            d1 = np.array([0.18 * side, -math.cos(a), math.sin(a)]); d1 /= np.linalg.norm(d1)
            el = sh + 0.24 * d1
            a2 = a + (0.35 if walking else 0.15)
            d2 = np.array([0.1 * side, -math.cos(a2), math.sin(a2)]); d2 /= np.linalg.norm(d2)
            wr = el + 0.22 * d2
            self.arms.append((sh, el, wr))
        # twin tails: trailing behind while walking + side sway
        trail = (-0.35 - 0.08 * math.sin(2 * phase)) if walking else (-0.05 + 0.03 * math.sin(phase))
        sway = (0.10 * math.sin(phase - 0.8)) if walking else 0.03 * math.sin(phase)
        self.tails = []
        for side in (-1, 1):
            base = self.H + np.array([0.47 * side, 0.24, -0.14])
            pts = [base]
            segs = [(0.20 * side, -0.28, -0.10), (0.03 * side, -0.52, -0.08), (-0.06 * side, -0.42, -0.04)]
            ang = 0.0
            for i, s in enumerate(segs):
                ang += trail * (0.4 + 0.3 * i)
                v = rot_x(s, ang) + np.array([sway * (i + 1) * 0.6, 0, 0])
                pts.append(pts[-1] + v)
            self.tails.append(pts)
        self.skirt_sway = 0.05 * math.sin(phase) if walking else 0.0

def scene(p, P):
    """Returns (distance, material) of every point p (N,3) in model space."""
    Hc = P.H
    q = p - Hc
    # --- skin: head, neck, arms, hands
    head = sd_ellipsoid(p, Hc, (0.55, 0.50, 0.52))
    neck = sd_capsule(p, (0, 1.17 + P.hip_y - 0.76, 0), Hc + (0, -0.35, 0), 0.075)
    skin = smin(head, neck, 0.05)
    for sh, el, wr in P.arms:
        skin = np.minimum(skin, smin(sd_capsule(p, sh, el, 0.058, 0.052), sd_capsule(p, el, wr, 0.052, 0.045), 0.03))
        skin = smin(skin, sd_ellipsoid(p, wr + (wr - el) * 0.25, (0.06, 0.07, 0.06)), 0.02)
    # --- hair
    cap = sd_ellipsoid(p, Hc + (0, 0.05, -0.05), (0.59, 0.57, 0.59))
    cut = sd_box(q, (0, -0.42, 0.62), (0.47, 0.62, 0.62))
    cap = np.maximum(cap, -cut)
    back = sd_capsule(p, Hc + (0, 0.05, -0.20), Hc + (0, -0.55, -0.27), 0.46, 0.26)
    hair = smin(cap, back, 0.08)
    for i, az in enumerate(np.linspace(-0.62, 0.62, 6)):
        def sp(el, r=0.60):
            return Hc + r * np.array([math.sin(az) * math.cos(el), math.sin(el), math.cos(az) * math.cos(el)])
        tip_el = 0.20 + 0.06 * ((i % 2) * 2 - 1)
        hair = np.minimum(hair, sd_capsule(p, sp(1.0), sp(tip_el, 0.57), 0.13, 0.03))
    for side in (-1, 1):
        hair = np.minimum(hair, sd_capsule(p, Hc + (0.43 * side, 0.12, 0.26), Hc + (0.47 * side, -0.45, 0.22), 0.10, 0.035))
    for pts in P.tails:
        radii = [0.09, 0.13, 0.10, 0.02]
        for a, b, r1, r2 in zip(pts[:-1], pts[1:], radii[:-1], radii[1:]):
            hair = smin(hair, sd_capsule(p, a, b, r1, r2), 0.06)
    # --- shirt (torso + short sleeves)
    tq = p * np.array([1, 1, 1.25])
    shirt = sd_capsule(tq, (0, P.hip_y + 0.14, 0), (0, P.hip_y + 0.44, 0), 0.20, 0.17) / 1.25
    for sh, el, wr in P.arms:
        shirt = smin(shirt, sd_capsule(p, sh, sh + (el - sh) * 0.55, 0.085, 0.075), 0.04)
    # --- skirt with sway
    sq = p.copy(); sq[:, 0] -= P.skirt_sway * (P.hip_y + 0.2 - p[:, 1]).clip(0, None)
    sq = sq * np.array([1, 1, 1.12])
    skirt = sd_capsule(sq, (0, P.hip_y + 0.20, 0), (0, P.hip_y - 0.15, 0), 0.20, 0.38) / 1.12
    skirt = np.maximum(skirt, sd_box(p, (0, P.hip_y + 0.02, 0), (1, 0.18, 1)))
    # --- legs (thigh-high socks via decal), shoes
    leg = None; shoe = None
    for hip, knee, ankle in P.legs:
        d = smin(sd_capsule(p, hip, knee, 0.078, 0.064), sd_capsule(p, knee, ankle, 0.064, 0.05), 0.02)
        leg = d if leg is None else np.minimum(leg, d)
        fwd = ankle - knee; fwd = np.array([0, 0, 1.0])
        s = sd_ellipsoid(p, ankle + (0, -0.04, 0.05), (0.075, 0.065, 0.12))
        shoe = s if shoe is None else np.minimum(shoe, s)
    # --- ribbons: chest + hair ties
    rib = sd_ellipsoid(p, (0, P.hip_y + 0.40, 0.17), (0.10, 0.05, 0.05))
    for pts in P.tails:
        rib = np.minimum(rib, sd_ellipsoid(p, pts[0] + (0, 0.04, 0), (0.09, 0.09, 0.12)))
    ds = np.stack([skin, hair, shirt, skirt, leg, shoe, rib])
    m = ds.argmin(0)
    return ds.min(0), m

# ---------------------------------------------------------------- decals (eyes, collar, pleats...)
def surface_color(p, m, n, P, ndl):
    col = np.array([PAL[k] for k in range(7)], float)[m]
    q = p - P.H
    # face (skin of the head, front)
    face = (m == SKIN) & (length(q) < 0.62) & (q[:, 2] > 0.15)
    for sx in (-1, 1):
        u = (q[:, 0] - sx * 0.205) / 0.11
        w = (q[:, 1] + 0.06) / 0.15
        e = u * u + w * w
        if P.blink:
            line = face & (np.abs(w + 0.05 - 0.35 * u * u) < 0.2) & (np.abs(u) < 1.05)
            col[line] = (60, 30, 50)
        else:
            inside = face & (e < 1)
            iris = np.stack([np.interp(w, [-1, 1], [190, 80]), np.interp(w, [-1, 1], [120, 40]),
                             np.interp(w, [-1, 1], [240, 150])], -1)
            col[inside] = iris[inside]
            pup = inside & (u * u + (w / 0.65) ** 2 < 0.16)
            col[pup] = (45, 15, 70)
            lash = face & (e < 1.35) & (w > 0.55)
            col[lash] = (45, 25, 45)
            hl = inside & ((u + 0.35 * sx) ** 2 + (w - 0.32) ** 2 < 0.1)
            col[hl] = (255, 255, 255)
            hl2 = inside & ((u - 0.3 * sx) ** 2 + (w + 0.45) ** 2 < 0.03)
            col[hl2] = (255, 255, 255)
        blush = face & (((q[:, 0] - sx * 0.30) / 0.09) ** 2 + ((q[:, 1] + 0.20) / 0.04) ** 2 < 1)
        col[blush] = col[blush] * 0.45 + np.array((255, 130, 150)) * 0.55
    mouth = face & (np.abs(q[:, 0]) < 0.05) & (np.abs(q[:, 1] + 0.25 - 5 * q[:, 0] ** 2) < 0.014)
    col[mouth] = (190, 60, 90)
    # sailor collar + cuffs
    sh = m == SHIRT
    rel_y = p[:, 1] - P.hip_y
    collar = sh & (rel_y > 0.33) & ((p[:, 2] < 0.02) | (rel_y > 0.40 - 0.0 * np.abs(p[:, 0])))
    col[collar] = PAL[SKIRT]
    stripe = collar & (np.abs(rel_y - 0.36) < 0.012)
    col[stripe] = (255, 255, 255)
    # pleats of the skirt
    sk = m == SKIRT
    ang = np.arctan2(p[:, 0], p[:, 2])
    pleat = sk & (np.sin(ang * 14) > 0.55)
    col[pleat] = col[pleat] * 0.78
    hem = sk & (p[:, 1] < P.hip_y - 0.09)
    col[hem & (np.abs(p[:, 1] - (P.hip_y - 0.11)) < 0.012)] = (255, 255, 255)
    # legs: skin above the sock (absolute zone), sock below
    lg = m == LEG
    thigh = lg & (p[:, 1] > P.hip_y - 0.20)
    col[thigh] = PAL[SKIN]
    # toon shading: 3 bands
    shade = np.where(ndl > 0.25, 1.0, np.where(ndl > -0.2, 0.80, 0.64))
    col = col * shade[:, None]
    tint = np.where(ndl > 0.25, 0, 1)[:, None] * np.array([-6, -10, 8])   # bluish shadows
    col = col + tint
    # anime hair shine (angel ring)
    hr = (m == HAIR) & (np.abs(q[:, 1] - 0.30 - 0.03 * np.sin(np.arctan2(q[:, 0], q[:, 2]) * 9)) < 0.03) & (ndl > 0.0)
    col[hr] = col[hr] * 0.4 + np.array((255, 235, 245)) * 0.6
    # rim light
    return np.clip(col, 0, 255)

def normal(p, P):
    e = 0.004
    ex = np.array([e, 0, 0]); ey = np.array([0, e, 0]); ez = np.array([0, 0, e])
    n = np.stack([scene(p + ex, P)[0] - scene(p - ex, P)[0],
                  scene(p + ey, P)[0] - scene(p - ey, P)[0],
                  scene(p + ez, P)[0] - scene(p - ez, P)[0]], -1)
    return n / np.maximum(length(n)[:, None], 1e-9)

def render(P, yaw):
    w, h = W * SS, H * SS
    ppu = PPU * SS
    jj, ii = np.meshgrid(np.arange(w), np.arange(h))
    sx = (jj + 0.5 - w / 2) / ppu
    sy = (FOOT_Y * SS - (ii + 0.5)) / ppu
    right = np.array([1.0, 0, 0]); up = np.array([0, math.cos(PITCH), -math.sin(PITCH)])
    view = np.array([0, -math.sin(PITCH), -math.cos(PITCH)])
    o = sx.reshape(-1, 1) * right + sy.reshape(-1, 1) * up - view * 6
    # into model space: rotate by -yaw around y
    c, s = math.cos(-yaw), math.sin(-yaw)
    R = np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])
    o = o @ R.T; v = R @ view
    N = o.shape[0]
    t = np.zeros(N); hit = np.zeros(N, bool); alive = np.ones(N, bool)
    for _ in range(110):
        idx = np.nonzero(alive)[0]
        if idx.size == 0: break
        d, _m = scene(o[idx] + v * t[idx, None], P)
        t[idx] += d * 0.9
        done = d < 0.002
        hit[idx[done]] = True
        far = t[idx] > 14
        alive[idx[done | far]] = False
    rgb = np.zeros((N, 3)); mat = -np.ones(N, int); depth = np.full(N, 99.0)
    hi = np.nonzero(hit)[0]
    p = o[hi] + v * t[hi, None]
    _, m = scene(p, P)
    n = normal(p, P)
    L = R @ (np.array([-0.45, 0.75, 0.55]) / np.linalg.norm([-0.45, 0.75, 0.55]))
    ndl = n @ L
    rgb[hi] = surface_color(p, m, n, P, ndl)
    # rim light on the silhouette
    rim = (1 + n @ v) ** 3
    rgb[hi] = np.clip(rgb[hi] + rim[:, None] * 18, 0, 255)
    mat[hi] = m; depth[hi] = t[hi]
    rgb = rgb.reshape(h, w, 3); mat = mat.reshape(h, w); depth = depth.reshape(h, w)
    # inner outlines: material change or depth jump
    edge = np.zeros((h, w), bool)
    for dy, dx in ((0, 1), (1, 0), (1, 1), (0, 2), (2, 0)):
        a = mat[:h - dy, :w - dx]; b = mat[dy:, dx:]
        da = depth[:h - dy, :w - dx]; db = depth[dy:, dx:]
        e = (a >= 0) & (b >= 0) & (((a != b) & ~(((a == SKIN) & (b == LEG)) | ((a == LEG) & (b == SKIN)))) | (np.abs(da - db) > 0.12))
        edge[:h - dy, :w - dx] |= e
    rgb[edge] = rgb[edge] * 0.35 + np.array((40, 20, 40)) * 0.65
    alpha = (mat >= 0).astype(float)
    # downsample (premultiplied)
    pm = rgb * alpha[..., None]
    pm = pm.reshape(H, SS, W, SS, 3).mean((1, 3)); al = alpha.reshape(H, SS, W, SS).mean((1, 3))
    col = pm / np.maximum(al[..., None], 1e-6)
    img = np.dstack([col, al * 255]).astype(np.uint8)
    im = Image.fromarray(img, "RGBA")
    # outer outline 1px
    a = im.getchannel("A").point(lambda x: 255 if x > 100 else 0)
    grow = a.filter(ImageFilter.MaxFilter(3))
    out = Image.new("RGBA", im.size, (45, 22, 45, 255)); out.putalpha(grow)
    out.alpha_composite(Image.composite(im, Image.new("RGBA", im.size, (0, 0, 0, 0)), a))
    return out

def all_frames():
    """frames[dir][k]: k=0..7 walk, 8 idle, 9 idle with blink. dir 0 = facing down (towards the viewer), clockwise towards the right... (yaw = dir*45 deg)"""
    frames = []
    for d in range(8):
        yaw = d * math.pi / 4
        fr = [render(Pose(2 * math.pi * k / N_WALK, True), yaw) for k in range(N_WALK)]
        fr.append(render(Pose(0.0, False), yaw))
        fr.append(render(Pose(0.0, False, blink=True), yaw))
        frames.append(fr)
        print("dir", d, "ok", flush=True)
    return frames

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 2 and sys.argv[2] == "quick":
        fr = [[render(Pose(2 * math.pi * k / N_WALK, k < 8, blink=(k == 9)), d * math.pi / 4) for k in (0, 2, 4, 9)] for d in (0, 2, 3, 4)]
    else:
        fr = all_frames()
    sheet = Image.new("RGBA", (W * len(fr[0]), H * len(fr)), (90, 160, 90, 255))
    for r, row in enumerate(fr):
        for c, f in enumerate(row): sheet.alpha_composite(f, (c * W, r * H))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(sys.argv[1])
