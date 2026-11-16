run(){ name=$1; keys=$2; OPS=out/$name.ops KEYS="$keys" ./test/sim; python3 test/render.py out/$name.ops out >/dev/null; rm -f out/$name.ops; }
