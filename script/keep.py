#!/usr/bin/env python3
"""
K-Means 聚类结果渲染脚本 —— 同时输出 SVG + PNG，支持大量簇的高区分度着色

用法:
  uv run script/render.py [时间戳] [--show]

示例:
  uv run script/render.py 20260607_135044          # 保存 SVG 和 PNG
  uv run script/render.py 20260607_135044 --show   # 保存并弹出窗口展示
  uv run script/render.py                          # 交互选择运行结果，保存 SVG 和 PNG

依赖 (uv 自动管理): numpy, matplotlib
"""

import os
import struct
import csv
import sys
import colorsys
import random
from pathlib import Path
from typing import List, Tuple

import numpy as np
from matplotlib.colors import ListedColormap


# ════════════════════════════════════════════════════════════
# 颜色生成工具 —— 用于大量簇的高区分度着色
# ════════════════════════════════════════════════════════════
def generate_distinct_colors(n: int, seed: int = 42) -> Tuple[List[Tuple[float, float, float]], ListedColormap]:
    """
    生成 n 种视觉区分度高的颜色 (RGB 三元组列表 以及 matplotlib ListedColormap)

    策略:
        - 在 HSV 色相环上等间距取 n 个色相
        - 饱和度在 0.7~1.0 之间轻微随机波动
        - 明度在 0.8~1.0 之间轻微随机波动
        - 固定随机种子保证每次生成的颜色一致
    """
    rng = random.Random(seed)
    colors = []
    for i in range(n):
        hue = i / n                          # 色相均匀分布
        saturation = 0.7 + rng.random() * 0.3  # 高饱和
        value      = 0.8 + rng.random() * 0.2  # 较高明度
        rgb = colorsys.hsv_to_rgb(hue, saturation, value)
        colors.append(rgb)
    cmap = ListedColormap(colors, name=f"custom_{n}")
    return colors, cmap


# ════════════════════════════════════════════════════════════
# 数据读取函数
# ════════════════════════════════════════════════════════════
def read_labels(bin_path: str):
    with open(bin_path, "rb") as f:
        n = struct.unpack("Q", f.read(8))[0]
        labels = np.frombuffer(f.read(), dtype=np.uint32)
        assert len(labels) == n, (
            f"labels.bin 数据不完整: 预期 {n} 个, 实际 {len(labels)} 个"
        )
    return labels


def read_render(csv_path: str):
    xs, ys, labs = [], [], []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            xs.append(float(row[0]))
            ys.append(float(row[1]))
            labs.append(int(row[2]))
    return np.array(xs), np.array(ys), np.array(labs)


def read_centers(csv_path: str):
    cxs, cys, ids = [], [], []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            cxs.append(float(row[0]))
            cys.append(float(row[1]))
            ids.append(int(row[2]))
    return np.array(cxs), np.array(cys), np.array(ids)


# ════════════════════════════════════════════════════════════
# 交互式选择运行结果文件夹
# ════════════════════════════════════════════════════════════
def pick_folder(results_dir: str) -> str:
    results_path = Path(results_dir)
    if not results_path.exists():
        print(f"Error: 目录不存在: {results_dir}")
        sys.exit(1)

    folders = sorted(
        [d.name for d in results_path.iterdir()
         if d.is_dir() and d.name != ".gitkeep"]
    )
    if not folders:
        print(f"Error: {results_dir}/ 下没有运行结果文件夹")
        sys.exit(1)

    print(f"\n  {'#':>3}  运行时间")
    print(f"  {'---':>3}  {'--------':>14}")
    for i, f in enumerate(folders):
        print(f"  {i+1:>3}. {f}")

    while True:
        try:
            choice = input(f"\n  选择 (1-{len(folders)}): ").strip()
            idx = int(choice) - 1
            if 0 <= idx < len(folders):
                return str(results_path / folders[idx])
        except ValueError:
            pass
        print(f"  请输入 1-{len(folders)} 之间的数字")


# ════════════════════════════════════════════════════════════
# 主渲染函数
# ════════════════════════════════════════════════════════════
def render(run_dir: str, show: bool = False):
    import matplotlib
    if not show:
        matplotlib.use("Agg")   # 无头保存模式
    import matplotlib.pyplot as plt

    run_path = Path(run_dir)
    timestamp = run_path.name

    labels_path  = run_path / "labels.bin"
    render_path  = run_path / "render.csv"
    centers_path = run_path / "centers.csv"
    result_path  = run_path / "result.txt"

    for p, name in [(labels_path, "labels.bin"),
                    (render_path, "render.csv"),
                    (centers_path, "centers.csv")]:
        if not p.exists():
            print(f"Error: 找不到 {name}")
            sys.exit(1)

    print(f"\n  Timestamp:     {timestamp}")
    print(f"  Dir:           {run_dir}")

    # 读取计算后端信息
    backend = "?"
    if result_path.exists():
        with open(result_path) as f:
            for line in f:
                if "计算后端" in line:
                    backend = line.split(":")[-1].strip()
                    break

    # 读取聚类数据
    labels = read_labels(str(labels_path))
    xs, ys, render_labels = read_render(str(render_path))
    cxs, cys, center_ids = read_centers(str(centers_path))

    k = len(cxs)
    print(f"  Clusters (K):  {k}")
    print(f"  Backend:       {backend}")
    print(f"  Render points: {len(xs):,}")
    print(f"  Total labels:  {len(labels):,}")

    # 根据实际簇数生成专用高区分度色板
    colors, cmap_custom = generate_distinct_colors(k)

    # 创建图形
    fig, ax = plt.subplots(figsize=(16, 12), dpi=150)

    # 绘制采样点
    ax.scatter(
        xs, ys, c=render_labels, s=1, cmap=cmap_custom,
        alpha=0.6, rasterized=True
    )

    # 绘制聚类中心
    ax.scatter(
        cxs, cys, c=range(k), cmap=cmap_custom,
        s=200, marker="X", edgecolors="white", linewidths=2,
        zorder=5
    )

    # ---- 图例处理：簇太多时只显示前 20 个，避免画面混乱 ----
    handles = []
    for i in range(k):
        handles.append(
            plt.Line2D(
                [0], [0], marker="o", color="w",
                markerfacecolor=colors[i],
                markersize=8, label=f"Cluster {i}"
            )
        )
    if k > 50:
        ax.legend(
            handles=handles[:20],
            title=f"Clusters (showing 20 of {k})",
            loc="best", fontsize=8
        )
    else:
        ax.legend(handles=handles, title="Clusters", loc="best", fontsize=10)

    # 标题与坐标轴
    ax.set_title(
        f"K-Means Clustering  (K={k}, {backend}, Points={len(labels):,})",
        fontsize=14
    )
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)

    plt.tight_layout()

    # ---- 同时保存 PNG 和 SVG ----
    out_dir = run_path
    base_name = f"clusters_{timestamp}"

    # 保存 PNG（栅格，有 DPI 概念）
    png_path = out_dir / f"{base_name}.png"
    fig.savefig(png_path, dpi=150)
    print(f"  Saved (PNG):   {png_path}")

    # 保存 SVG（矢量，不受 DPI 影响）
    svg_path = out_dir / f"{base_name}.svg"
    fig.savefig(svg_path)
    print(f"  Saved (SVG):   {svg_path}")

    # 是否弹出展示窗口
    if show:
        print("  正在显示聚类结果，关闭窗口后退出...")
        plt.show()
    else:
        plt.close(fig)


# ════════════════════════════════════════════════════════════
# 入口
# ════════════════════════════════════════════════════════════
if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    results_dir = str(project_dir / "results")

    # 简单命令行解析：支持位置参数时间戳 和 --show 标志
    timestamp_arg = ""
    show_flag = False
    for arg in sys.argv[1:]:
        if arg == "--show":
            show_flag = True
        elif not arg.startswith("--"):
            timestamp_arg = arg

    # 确定运行结果文件夹
    if timestamp_arg:
        run_dir = os.path.join(results_dir, timestamp_arg)
        if not os.path.isdir(run_dir):
            print(f"Error: 找不到运行结果: {run_dir}")
            sys.exit(1)
    else:
        run_dir = pick_folder(results_dir)

    render(run_dir, show=show_flag)