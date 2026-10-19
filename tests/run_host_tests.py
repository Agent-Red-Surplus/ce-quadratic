from pathlib import Path
import os, shlex, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
cc=shlex.split(os.environ.get("CC","cc"))
flags=["-std=c99","-Wall","-Wextra","-Werror","-Wno-misleading-indentation"]
def run(cmd): subprocess.run(cmd,cwd=root,check=True)
with tempfile.TemporaryDirectory() as temp:
    temp=Path(temp)
    sources=["src/quadratic_math.c","src/factoring_math.c","tests/reference_quadratic.c","tests/test_math.c"]
    for name,defs in [("math",[]),("budget",["-DFACTOR_WORK_BUDGET=3UL","-DTEST_FORCED_BUDGET"]),("cancel",["-DFACTOR_POLL_MASK=0UL","-DTEST_FORCED_CANCEL"])]:
        exe=temp/name
        run(cc+flags+["-O2","-Isrc"]+defs+sources+["-o",str(exe)])
        run([str(exe)])
    run(cc+flags+["-Isrc","-Itests/host_stubs"]+[str(p.relative_to(root)) for p in sorted((root/"src").glob("*.c"))]+["tests/host_stubs/host_api.c","-o",str(temp/"link-check")])
    print("Host source/link check passed with API stubs; not calculator emulation.")
