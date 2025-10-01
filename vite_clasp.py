Import("env")
import sys

IS_WINDOWS = sys.platform.startswith("win")
IS_LINUX = sys.platform.startswith("linux")
IS_MAC = sys.platform.startswith("darwin")

print("Solid+TS integration enabled")
if (IS_WINDOWS):
    env.Execute("build_web.cmd")
else:
    env.Execute("bash build_web.sh")

print("ClASP Suite integration enabled")

import shutil
python_cmd = "python3" if shutil.which("python3") else "python"
env.Execute(f"{python_cmd} clasptree.py ./web/dist -o ./include/httpd_content.h -p httpd_ -E ./include/httpd_epilogue.h -s context -b httpd_send_block -e httpd_send_expr -H extended")