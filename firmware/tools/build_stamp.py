from pathlib import Path
import sys
path = Path(sys.argv[1])
text = '\n'.join(sys.argv[2:]) + '\n'
path.parent.mkdir(parents=True, exist_ok=True)
if not path.exists() or path.read_text() != text:
    path.write_text(text)
