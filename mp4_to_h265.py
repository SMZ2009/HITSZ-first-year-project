#!/usr/bin/env python3
"""
mp4_to_h265.py

将 MP4 文件中的视频流转换为 H.265/HEVC 原始流（.h265 文件，Annex-B 格式）。

用法：
    python mp4_to_h265.py input.mp4 output.h265
"""

import subprocess
import sys
import shutil
from pathlib import Path

def mp4_to_h265(input_path: str, output_path: str):
    """
    使用 ffmpeg 提取 MP4 中的视频流并转换为 H.265/HEVC .h265 文件。
    """
    # 检查 ffmpeg 是否可用
    if not shutil.which("ffmpeg"):
        raise RuntimeError("ffmpeg 未安装或不在 PATH 中。请先安装 ffmpeg。")

    # 检查输入文件是否存在
    input_file = Path(input_path)
    if not input_file.is_file():
        raise FileNotFoundError(f"输入文件不存在: {input_path}")

    # 构造 ffmpeg 命令
    cmd = [
        "ffmpeg", "-y",
        "-i", str(input_file),
        "-c:v", "libx265",  # 编码为 H.265/HEVC
        "-an",               # 不输出音频
        "-f", "hevc",        # 输出原始 H.265 流
        str(output_path)
    ]

    # 执行命令
    print(f"正在转换 {input_file} -> {output_path} ...")
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print("ffmpeg 出错：")
        print(result.stderr)
        raise RuntimeError("转换失败！")

    print(f"转换完成，输出文件: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("用法: python mp4_to_h265.py input.mp4 output.h265")
        sys.exit(1)

    input_mp4 = sys.argv[1]
    output_h265 = sys.argv[2]

    try:
        mp4_to_h265(input_mp4, output_h265)
    except Exception as e:
        print("错误:", e)
        sys.exit(1)