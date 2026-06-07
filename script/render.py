#!/usr/bin/env python3
"""
K-Means 聚类结果渲染脚本

用途:
  读取 results/ 下某次运行的结果文件，绘制散点图。

用法:
  # 渲染最新一次运行结果
  python3 script/render.py

  # 指定某次运行的时间戳文件夹
  python3 script/render.py 20260607_112634

  # 指定 results 目录（默认 projects/results/）
  python3 script/render.py 20260607_112634 --results-dir /path/to/results

输出:
  脚本所在目录下的 clusters_时间戳.png

依赖:
  pip install matplotlib numpy
"""

import argparse
import os
import sys
import struct
import csv
from pathlib import Path

import numpy as np


# ============================================================
# 读取 labels.bin
# ============================================================
def read_labels(bin_path: str) -> np.ndarray:
    """读取 labels.bin，返回 uint32 数组"""
    with open(bin_path, "rb") as f:
        n = struct.unpack("Q", f.read(8))[0]
        labels = np.frombuffer(f.read(), dtype=np.uint32)
        assert len(labels) == n, (
            f"labels.bin 数据不完整: 预期 {n} 个, 实际 {len(labels)} 个"
        )
    return labels


# ============================================================
# 读取 render.csv
# ============================================================
def read_render(csv_path: str) -> tuple:
    """读取 render.csv，返回 (xs, ys, labels) 三元组"""
    xs, ys, labs = [], [], []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        header = next(reader)  # x,y,label
        for row in reader:
            xs.append(float(row[0]))
            ys.append(float(row[1]))
            labs.append(int(row[2]))
    return np.array(xs), np.array(ys), np.array(labs)


# ============================================================
# 读取 centers.csv
# ============================================================
def read_centers(csv_path: str) -> tuple:
    """读取 centers.csv，返回 (cxs, cys, cluster_ids) 三元组"""
    cxs, cys, ids = [], [], []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        header = next(reader)  # cx,cy,cluster
        for row in reader:
            cxs.append(float(row[0]))
            cys.append(float(row[1]))
            ids.append(int(row[2]))
    return np.array(cxs), np.array(cys), np.array(ids)


# ============================================================
# 找最新一次运行结果
# ============================================================
def find_latest(results_dir: str) -> str:
    """在 results/ 下找时间戳最晚的文件夹名"""
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

    latest = folders[-1]
    return str(results_path / latest)


# ============================================================
# 主渲染函数
# ============================================================
def render(run_dir: str, output_path: str = None):
    """
    渲染一次运行结果。

    参数:
      run_dir:    运行结果文件夹路径（含 result.txt / labels.bin / centers.csv / render.csv）
      output_path: 输出图片路径（None 则自动生成）
    """
    run_path = Path(run_dir)
    if not run_path.exists():
        print(f"Error: 运行结果目录不存在: {run_dir}")
        sys.exit(1)

    # 解析时间戳（从文件夹名）
    timestamp = run_path.name

    # 文件路径
    labels_path  = run_path / "labels.bin"
    render_path  = run_path / "render.csv"
    centers_path = run_path / "centers.csv"
    result_path  = run_path / "result.txt"

    if not labels_path.exists():
        print(f"Error: 找不到 {labels_path}")
        sys.exit(1)
    if not render_path.exists():
        print(f"Error: 找不到 {render_path}")
        sys.exit(1)
    if not centers_path.exists():
        print(f"Error: 找不到 {centers_path}")
        sys.exit(1)

    print(f"  运行时间:     {timestamp}")
    print(f"  结果目录:     {run_dir}")

    # 从 result.txt 读取后端信息
    backend = "?"
    if result_path.exists():
        with open(result_path) as f:
            for line in f:
                if "计算后端" in line:
                    backend = line.split(":")[-1].strip()
                    break

    # --- 读取数据 ---
    labels = read_labels(str(labels_path))
    xs, ys, render_labels = read_render(str(render_path))
    cxs, cys, center_ids = read_centers(str(centers_path))

    # K 值
    k = len(cxs)
    print(f"  簇数 (K):     {k}")
    print(f"  后端:         {backend}")
    print(f"  渲染点数:     {len(xs)}")
    print(f"  全部标签:     {len(labels)} 个")

    # --- 导入 matplotlib（延迟导入，提速 --help） ---
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    # --- 渲染 ---
    fig, ax = plt.subplots(figsize=(16, 12), dpi=150)

    # 散点（按 label 着色）
    scatter = ax.scatter(
        xs, ys, c=render_labels, s=1, cmap="tab10",
        alpha=0.6, rasterized=True
    )

    # 中心点（大号 + 十字）
    ax.scatter(
        cxs, cys, c=range(k), cmap="tab10",
        s=200, marker="X", edgecolors="white", linewidths=2,
        zorder=5
    )

    # 自动调整图例
    handles = []
    for i in range(k):
        handles.append(
            plt.Line2D(
                [0], [0], marker="o", color="w",
                markerfacecolor=plt.cm.tab10(i / 10),
                markersize=8, label=f"Cluster {i}"
            )
        )
    ax.legend(handles=handles, title="聚类", loc="best", fontsize=10)

    ax.set_title(
        f"K-Means Clustering  (K={k}, 后端={backend}, 点数={len(labels):,})",
        fontsize=14
    )
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)

    plt.tight_layout()

    # --- 保存 ---
    if output_path is None:
        output_path = f"clusters_{timestamp}.png"

    plt.savefig(output_path, dpi=150)
    plt.close(fig)

    print(f"  图片已保存:   {output_path}")
    print(f"  分辨率:       150 dpi, {fig.get_size_inches()[0]:.0f}x{fig.get_size_inches()[1]:.0f} in")


# ============================================================
# 入口
# ============================================================
def main():
    parser = argparse.ArgumentParser(
        description="K-Means 聚类结果渲染脚本",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
示例:
  # 渲染最新结果
  python3 script/render.py

  # 渲染指定时间戳的运行
  python3 script/render.py 20260607_112634

  # 指定自定义 results 目录
  python3 script/render.py 20260607_112634 --results-dir /path/to/results

  # 指定输出路径
  python3 script/render.py -o my_plot.png
"""
    )
    parser.add_argument(
        "timestamp", nargs="?",
        help="运行结果的时间戳文件夹名 (如 20260607_112634)，缺省则用最新一次"
    )
    parser.add_argument(
        "--results-dir", "-r", default=None,
        help="results 目录路径（缺省自动检测项目根目录下的 results/）"
    )
    parser.add_argument(
        "--output", "-o", default=None,
        help="输出图片路径（缺省: clusters_时间戳.png）"
    )

    args = parser.parse_args()

    # ---- 确定 results 目录 ----
    if args.results_dir:
        results_dir = args.results_dir
    else:
        # 自动检测项目根目录
        script_dir = Path(__file__).resolve().parent  # script/
        project_dir = script_dir.parent                # 项目根
        results_dir = str(project_dir / "results")

    # ---- 确定运行结果目录 ----
    if args.timestamp:
        run_dir = os.path.join(results_dir, args.timestamp)
        if not os.path.isdir(run_dir):
            print(f"Error: 找不到运行结果: {run_dir}")
            sys.exit(1)
    else:
        run_dir = find_latest(results_dir)

    # ---- 渲染 ----
    render(run_dir, args.output)


if __name__ == "__main__":
    main()
