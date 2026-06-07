#!/usr/bin/env python3
"""
K-Means 聚类结果展示脚本 (仅显示，不保存图片)

用法:
  uv run script/show.py [时间戳]
  例: uv run script/show.py 20260607_135044
  如果不提供时间戳，则交互选择已有的运行结果文件夹。
"""

import os
import struct
import csv
import sys
from pathlib import Path


# ════════════════════════════════════════════════════════════
# 读取 labels.bin
# ════════════════════════════════════════════════════════════
def read_labels(bin_path: str):
    import numpy as np
    with open(bin_path, "rb") as f:
        n = struct.unpack("Q", f.read(8))[0]
        labels = np.frombuffer(f.read(), dtype=np.uint32)
        assert len(labels) == n, (
            f"labels.bin 数据不完整: 预期 {n} 个, 实际 {len(labels)} 个"
        )
    return labels


# ════════════════════════════════════════════════════════════
# 读取 render.csv
# ════════════════════════════════════════════════════════════
def read_render(csv_path: str):
    import numpy as np
    xs, ys, labs = [], [], []
    with open(csv_path, "r") as f:
        reader = csv.reader(f)
        next(reader)
        for row in reader:
            xs.append(float(row[0]))
            ys.append(float(row[1]))
            labs.append(int(row[2]))
    return np.array(xs), np.array(ys), np.array(labs)


# ════════════════════════════════════════════════════════════
# 读取 centers.csv
# ════════════════════════════════════════════════════════════
def read_centers(csv_path: str):
    import numpy as np
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
# 交互式选择
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
# 展示 (仅显示，不保存)
# ════════════════════════════════════════════════════════════
def show(run_dir: str):
    import matplotlib.pyplot as plt
    import numpy as np

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

    # 读取后端信息
    backend = "?"
    if result_path.exists():
        with open(result_path) as f:
            for line in f:
                if "计算后端" in line:
                    backend = line.split(":")[-1].strip()
                    break

    # 读取数据
    labels = read_labels(str(labels_path))
    xs, ys, render_labels = read_render(str(render_path))
    cxs, cys, center_ids = read_centers(str(centers_path))

    k = len(cxs)
    print(f"  Clusters (K):  {k}")
    print(f"  Backend:       {backend}")
    print(f"  Render points: {len(xs):,}")
    print(f"  Total labels:  {len(labels):,}")

    # 创建图形
    fig, ax = plt.subplots(figsize=(16, 12), dpi=150)

    # 绘制采样点
    ax.scatter(
        xs, ys, c=render_labels, s=1, cmap="tab10",
        alpha=0.6, rasterized=True
    )

    # 绘制聚类中心
    ax.scatter(
        cxs, cys, c=range(k), cmap="tab10",
        s=200, marker="X", edgecolors="white", linewidths=2,
        zorder=5
    )

    # 图例
    handles = []
    for i in range(k):
        handles.append(
            plt.Line2D(
                [0], [0], marker="o", color="w",
                markerfacecolor=plt.cm.tab10(i / 10),
                markersize=8, label=f"Cluster {i}"
            )
        )
    ax.legend(handles=handles, title="Clusters", loc="best", fontsize=10)

    # 标题与标签
    ax.set_title(
        f"K-Means Clustering  (K={k}, {backend}, Points={len(labels):,})",
        fontsize=14
    )
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)

    plt.tight_layout()

    # 直接显示，不保存文件
    print("  正在显示聚类结果，关闭窗口后退出...")
    plt.show()


# ════════════════════════════════════════════════════════════
# 入口
# ════════════════════════════════════════════════════════════
if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_dir = script_dir.parent
    results_dir = str(project_dir / "results")

    # 如果命令行提供了时间戳，直接使用；否则交互选择
    if len(sys.argv) > 1:
        timestamp = sys.argv[1]
        run_dir = os.path.join(results_dir, timestamp)
        if not os.path.isdir(run_dir):
            print(f"Error: 找不到运行结果: {run_dir}")
            sys.exit(1)
    else:
        run_dir = pick_folder(results_dir)

    show(run_dir)