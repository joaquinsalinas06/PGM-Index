import csv
import math
import os
import statistics as st
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "results")
FIG = os.path.join(HERE, "..", "informe", "figuras")
TAB = os.path.join(HERE, "..", "informe", "tablas")
os.makedirs(FIG, exist_ok=True)
os.makedirs(TAB, exist_ok=True)

from matplotlib import font_manager
LM = "/usr/local/texlive/2026basic/texmf-dist/fonts/opentype/public/lm/lmroman10-regular.otf"
if os.path.exists(LM):
    font_manager.fontManager.addfont(LM)
plt.rcParams.update({
    "font.family": "serif", "font.serif": ["Latin Modern Roman", "DejaVu Serif"], "mathtext.fontset": "cm",
    "axes.formatter.use_mathtext": True, "font.size": 9, "axes.titlesize": 9,
    "axes.labelsize": 9, "legend.fontsize": 7.5, "xtick.labelsize": 8, "ytick.labelsize": 8,
    "axes.spines.top": False, "axes.spines.right": False, "axes.grid": True,
    "grid.color": "#dddddd", "grid.linewidth": 0.5, "lines.linewidth": 1.3,
    "lines.markersize": 3.5, "savefig.bbox": "tight", "figure.dpi": 150,
})
C = {"pgm": "#1f5fa8", "bplus": "#d9822b", "binaria": "#666666", "buffered": "#2e8b57",
     "logarithmic": "#7b4fa0", "bplus_dyn": "#d9822b", "pgm_static": "#1f5fa8"}
NAMES = {"seq": "seq", "uniform": "uniforme", "normal": "normal", "lognormal": "lognormal",
         "exponential": "exponencial", "zipf": "zipf", "books": "books", "fb": "fb", "osm": "osm",
         "wiki": "wiki", "friendster": "friendster", "patho_loggap": "pat.\\ loggap",
         "patho_pareto": "pat.\\ pareto", "patho_ruler": "pat.\\ regla"}
PLAIN = {k: v.replace("\\ ", " ") for k, v in NAMES.items()}
ORDER = list(NAMES)
FAMILY = {"seq": "sintético", "uniform": "sintético", "normal": "sintético", "lognormal": "sintético",
          "exponential": "sintético", "zipf": "sintético", "books": "real", "fb": "real",
          "osm": "real", "wiki": "real", "friendster": "real",
          "patho_loggap": "patológico", "patho_pareto": "patológico", "patho_ruler": "patológico"}


def read(name):
    with open(os.path.join(RES, name)) as f:
        return list(csv.DictReader(f))


def save(fig, name):
    fig.savefig(os.path.join(FIG, name + ".pdf"))
    plt.close(fig)


def fmt_bytes(b):
    b = float(b)
    for unit in ["B", "KB", "MB", "GB"]:
        if b < 1024 or unit == "GB":
            return f"{b:.0f}\\,{unit}"
        b /= 1024


def fmt_int(x):
    return f"{int(x):,}".replace(",", "\\,")


seg = read("segments.csv")
S = {(r["dataset"], int(r["eps"])): r for r in seg}
EPS = sorted({int(r["eps"]) for r in seg})
DS = [d for d in ORDER if (d, 64) in S]

fig, ax = plt.subplots(1, 2, figsize=(7.0, 2.7))
show = ["uniform", "lognormal", "zipf", "books", "fb", "osm", "wiki", "patho_pareto", "patho_ruler"]
cmap = plt.get_cmap("tab10")
for i, d in enumerate(show):
    ys = [int(S[(d, e)]["m"]) for e in EPS]
    ax[0].loglog(EPS, ys, "o-", color=cmap(i), label=PLAIN[d])
    ax[1].semilogx(EPS, [int(S[(d, e)]["m"]) / float(S[(d, e)]["bound"]) for e in EPS], "o-", color=cmap(i))
n0 = int(S[(DS[0], 64)]["n"])
ax[0].loglog(EPS, [n0 / (2 * e) for e in EPS], "k--", lw=1, label=r"cota $n/(2\varepsilon)$")
ax[0].set_xlabel(r"$\varepsilon$"); ax[0].set_ylabel("segmentos $m$ (nivel base)")
ax[1].set_xlabel(r"$\varepsilon$"); ax[1].set_ylabel(r"$m \,/\, (n/2\varepsilon)$")
ax[1].axhline(1, color="k", ls="--", lw=1)
ax[1].set_ylim(bottom=0)
for a in ax:
    a.set_xticks(EPS); a.set_xticklabels([str(e) for e in EPS], rotation=45)
    a.minorticks_off()
h, l = ax[0].get_legend_handles_labels()
fig.legend(h, l, ncol=5, frameon=False, loc="upper center", bbox_to_anchor=(0.5, 1.13))
fig.tight_layout()
save(fig, "segmentos")

with open(os.path.join(TAB, "segmentos.tex"), "w") as f:
    for d in DS:
        r = S[(d, 64)]
        f.write(f"{NAMES[d]} & {FAMILY[d]} & {fmt_int(r['m'])} & {r['height']} & "
                f"{fmt_bytes(r['bytes'])} \\\\\n")
worst = max(int(r["m"]) / float(r["bound"]) for r in seg)
with open(os.path.join(TAB, "valores.tex"), "w") as f:
    f.write(f"\\newcommand{{\\maxratio}}{{{worst:.3f}}}\n")

lk = read("lookup.csv")
L = defaultdict(list)
B = {}
for r in lk:
    k = (r["dataset"], r["index"], int(r["param"]))
    L[k].append(float(r["ns"]))
    B[k] = int(r["bytes"])


def med(k):
    return st.median(L[k])


def spread(k):
    return min(L[k]), max(L[k])


CAPS = sorted({k[2] for k in L if k[1] == "bplus"})
fig, ax = plt.subplots(1, 2, figsize=(7.0, 2.6))
show2 = ["uniform", "books", "fb", "osm", "patho_ruler"]
for i, d in enumerate(show2):
    ax[0].loglog(EPS, [B[(d, "pgm", e)] for e in EPS], "o-", color=cmap(i), label=PLAIN[d])
    m = [med((d, "pgm", e)) for e in EPS]
    lo = [m[j] - spread((d, "pgm", e))[0] for j, e in enumerate(EPS)]
    hi = [spread((d, "pgm", e))[1] - m[j] for j, e in enumerate(EPS)]
    ax[1].plot(EPS, m, "o-", color=cmap(i), label=PLAIN[d])
ax[1].set_xscale("log")
ax[0].set_xlabel(r"$\varepsilon$"); ax[0].set_ylabel("tamaño del índice (bytes)")
ax[1].set_xlabel(r"$\varepsilon$"); ax[1].set_ylabel("latencia de lookup (ns)")
for a in ax:
    a.set_xticks(EPS); a.set_xticklabels([str(e) for e in EPS], rotation=45); a.minorticks_off()
ax[0].legend(frameon=False, fontsize=7)
fig.tight_layout()
save(fig, "epsilon")

panel = ["uniform", "books", "fb", "osm", "wiki", "patho_ruler"]
fig, axs = plt.subplots(2, 3, figsize=(7.0, 4.4))
for a, d in zip(axs.flat, panel):
    a.plot([B[(d, "pgm", e)] for e in EPS], [med((d, "pgm", e)) for e in EPS], "o-", color=C["pgm"], label=r"PGM ($\varepsilon$)")
    a.plot([B[(d, "bplus", c)] for c in CAPS], [med((d, "bplus", c)) for c in CAPS], "s-", color=C["bplus"], label="B+ (capacidad)")
    a.axhline(med((d, "binaria", 0)), color=C["binaria"], ls="--", lw=1, label="búsqueda binaria")
    for e in (8, 64, 512):
        a.annotate(str(e), (B[(d, "pgm", e)], med((d, "pgm", e))), fontsize=6, color=C["pgm"],
                   xytext=(2, 3), textcoords="offset points")
    for c in (8, 64, 1024):
        a.annotate(str(c), (B[(d, "bplus", c)], med((d, "bplus", c))), fontsize=6, color=C["bplus"],
                   xytext=(2, -8), textcoords="offset points")
    a.set_xscale("log"); a.set_title(PLAIN[d]); a.set_ylim(bottom=0)
for a in axs[1]:
    a.set_xlabel("tamaño del índice (bytes)")
for a in axs[:, 0]:
    a.set_ylabel("latencia (ns)")
axs[0, 0].legend(frameon=False, fontsize=6.5, loc="lower left")
fig.tight_layout()
save(fig, "pareto")

with open(os.path.join(TAB, "comparacion.tex"), "w") as f:
    for d in DS:
        ratio = B[(d, "bplus", 64)] / B[(d, "pgm", 64)]
        f.write(f"{NAMES[d]} & {med((d, 'binaria', 0)):.0f} & {med((d, 'pgm', 64)):.0f} & "
                f"{med((d, 'bplus', 64)):.0f} & {fmt_bytes(B[(d, 'pgm', 64)])} & "
                f"{fmt_bytes(B[(d, 'bplus', 64)])} & {fmt_int(round(ratio))}$\\times$ \\\\\n")

sz = read("sizes.csv")
Z = defaultdict(list)
ZB = {}
for r in sz:
    k = (r["dataset"], int(r["n"]), r["index"])
    Z[k].append(float(r["ns"]))
    ZB[k] = int(r["bytes"])
for r in lk:
    if r["index"] == "binaria" or r["param"] == "64":
        k = (r["dataset"], int(r["n"]), r["index"])
        Z[k].append(float(r["ns"]))
        ZB[k] = int(r["bytes"])
NS = sorted({k[1] for k in Z})
fig, axs = plt.subplots(1, 4, figsize=(7.0, 2.2), sharey=True)
for a, d in zip(axs, ["uniform", "books", "osm", "patho_ruler"]):
    for idx, lab, mk in [("binaria", "binaria", "^"), ("pgm", r"PGM $\varepsilon$=64", "o"), ("bplus", "B+ cap=64", "s")]:
        a.plot(NS, [st.median(Z[(d, n, idx)]) for n in NS], mk + "-", color=C[idx], label=lab)
    a.set_xscale("log"); a.set_title(PLAIN[d]); a.set_xlabel("$n$"); a.minorticks_off()
axs[0].set_ylabel("latencia (ns)"); axs[0].legend(frameon=False, fontsize=6.5)
fig.tight_layout()
save(fig, "tamanos")
with open(os.path.join(TAB, "tamanos.tex"), "w") as f:
    for d in DS:
        cells = []
        for n in NS:
            p = Z[(d, n, "pgm")]
            lose = min(p) > min(max(Z[(d, n, "binaria")]), max(Z[(d, n, "bplus")]))
            pm = f"{st.median(p):.0f}"
            cells.append(" & ".join([f"{st.median(Z[(d, n, 'binaria')]):.0f}",
                                     f"\\textbf{{{pm}}}" if lose else pm,
                                     f"{st.median(Z[(d, n, 'bplus')]):.0f}"]))
        f.write(f"{NAMES[d]} & " + " & ".join(cells) + " \\\\\n")

dy = read("dynamic.csv")
G = defaultdict(list)
for r in dy:
    G[(r["dataset"], r["index"], int(r["cap"]))].append(r)
DDS = [d for d in ORDER if any(k[0] == d for k in G)]
CONF = [("buffered", 256, "Buffered cap=256"), ("buffered", 4096, "Buffered cap=4096"),
        ("logarithmic", 256, "Logarítmico cap=256"), ("logarithmic", 4096, "Logarítmico cap=4096"),
        ("bplus_dyn", 64, "B+ dinámico cap=64"), ("pgm_static", 64, "PGM estático (ref.)")]
COLS = {("buffered", 256): "#2e8b57", ("buffered", 4096): "#8fd1a8", ("logarithmic", 256): "#7b4fa0",
        ("logarithmic", 4096): "#c3a6dd", ("bplus_dyn", 64): "#d9822b", ("pgm_static", 64): "#1f5fa8"}


def gm(d, i, c, col):
    return st.median(float(r[col]) for r in G[(d, i, c)])


fig, axs = plt.subplots(1, 3, figsize=(7.0, 2.6))
w = 0.8 / len(CONF)
for j, (i, c, lab) in enumerate(CONF):
    xs = [k + (j - len(CONF) / 2 + 0.5) * w for k in range(len(DDS))]
    if i != "pgm_static":
        axs[0].bar(xs, [gm(d, i, c, "ins_avg_ns") for d in DDS], w, color=COLS[(i, c)], label=lab)
    axs[1].bar(xs, [gm(d, i, c, "look_med_ns") for d in DDS], w, color=COLS[(i, c)], label=lab)
    if i != "pgm_static":
        axs[2].bar(xs, [gm(d, i, c, "ins_max_ns") / 1e3 for d in DDS], w, color=COLS[(i, c)], label=lab)
axs[0].set_yscale("log"); axs[0].set_ylabel("inserción, costo medio (ns)")
axs[1].set_ylabel("lookup tras insertar (ns)")
axs[2].set_yscale("log"); axs[2].set_ylabel(r"peor inserción ($\mu$s)")
for a in axs:
    a.set_xticks(range(len(DDS))); a.set_xticklabels([PLAIN[d] for d in DDS], rotation=45, ha="right")
    a.grid(axis="x", visible=False)
h, l = axs[1].get_legend_handles_labels()
fig.legend(h, l, ncol=3, frameon=False, loc="upper center", bbox_to_anchor=(0.5, 1.12), fontsize=7)
fig.tight_layout()
save(fig, "dinamicos")
with open(os.path.join(TAB, "dinamicos.tex"), "w") as f:
    for d in DDS:
        for j, (i, c, lab) in enumerate(CONF):
            first = f"\\multirow{{{len(CONF)}}}{{*}}{{{NAMES[d]}}}" if j == 0 else ""
            if i == "pgm_static":
                f.write(f"{first} & {lab} & -- & -- & -- & {gm(d, i, c, 'look_med_ns'):.0f} & "
                        f"{gm(d, i, c, 'ram_bytes') / 2**20:.1f} & -- & -- \\\\\n")
                continue
            f.write(f"{first} & {lab} & {gm(d, i, c, 'ins_avg_ns'):,.0f} & {gm(d, i, c, 'ins_p99_ns'):,.0f} & "
                    f"{gm(d, i, c, 'ins_max_ns') / 1e6:.1f} & {gm(d, i, c, 'look_med_ns'):.0f} & "
                    f"{gm(d, i, c, 'ram_bytes') / 2**20:.1f} & {int(gm(d, i, c, 'maint'))} & "
                    f"{gm(d, i, c, 'moved_keys') / 1e6:,.1f} \\\\\n".replace(",", "\\,"))
        if d != DDS[-1]:
            f.write("\\midrule\n")
import re
for name in os.listdir(TAB):
    path = os.path.join(TAB, name)
    txt = open(path).read()
    if name != "valores.tex":
        txt = re.sub(r"(\d)\.(\d)", r"\1{,}\2", txt)
    else:
        txt = txt.replace(".", "{,}")
    open(path, "w").write(txt)
print("figuras y tablas generadas")
