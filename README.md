# SpinalVoodoo

## Scala CLI Commands

```bash
# Run tests
scala-cli test .

# Format code
scalafmt

# Compile
scala-cli compile .
```

## Trace Player

Replay Voodoo trace files and watch triangles render in real-time:

```bash
scala-cli run . -- \
  --trace /path/to/voodoo_trace.bin \
  [--index /path/to/voodoo_trace.bin.idx] \
  [--frame N] \
  [--timing accurate|freerun] \
  [--resolution WxH]
```

**Options:**
- `--trace` - Binary trace file (required)
- `--index` - Frame index file (auto-detected if `.bin.idx` exists)
- `--frame` - Render specific frame number (requires index)
- `--timing` - `accurate` for cycle-accurate, `freerun` for max speed (default: freerun)
- `--resolution` - Display resolution (default: 640x480)
