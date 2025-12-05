#!/usr/bin/env python3
"""
生成一个简单可播放 MP4 示例文件
"""

import subprocess
from pathlib import Path

def generate_sample_mp4(output_path="./data/sample.mp4"):
    output_file = Path(output_path)
    cmd = [
        "ffmpeg",
        "-y",                          # 覆盖已存在文件
        "-f", "lavfi",
        "-i", "color=c=blue:s=320x240:d=1:r=30",  # 蓝色背景，2秒，30fps
        "-c:v", "libx264",
        "-pix_fmt", "yuv420p",         # 保证兼容性
        str(output_file)
    ]
    print(f"生成示例 MP4 文件: {output_file}")
    subprocess.run(cmd, check=True)
    print("生成完成！")

if __name__ == "__main__":
    generate_sample_mp4("sample.mp4")
