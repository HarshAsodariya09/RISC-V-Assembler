from flask import Flask, request, render_template_string
import subprocess
import os

app = Flask(__name__)

# Configuration
KNOBS = [
    ('knob1', 'pipelining'),
    ('knob2', 'data forwarding'),
    ('knob3', 'register state'),
    ('knob4', 'pipeline registers'),
]
BRANCH_KNOB = ('knob6', 'branch prediction')

def get_register_file():
    reg_file = {}
    try:
        with open("register.txt", "r", encoding="utf-8") as f:
            for line in f:
                parts = line.strip().split('=')
                if len(parts) == 2:
                    name = parts[0].strip()
                    value = parts[1].strip()
                    reg_file[name] = value
    except Exception as e:
        reg_file["error"] = f"Could not read register.txt: {e}"
    return reg_file

def read_file(path):
    try:
        return open(path).read()
    except:
        return f"[Error: {path} not found]"

TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>RISC-V Simulator</title>
  <style>
    :root {
      --bg: #1e1e2f;
      --fg: #e0def4;
      --accent: #f6c177;
      --card: #2e2e42;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--fg);
      font-family: 'Segoe UI', sans-serif;
    }
    header {
      background: var(--card);
      text-align: center;
      padding: 2rem;
      box-shadow: 0 2px 8px rgba(0,0,0,0.5);
    }
    header h1 {
      color: var(--accent);
      font-size: 3rem;
      letter-spacing: 1px;
      text-decoration: underline;
    }
    main {
      max-width: 960px;
      margin: 2rem auto;
      padding: 0 1rem;
    }
    form {
      background: var(--card);
      padding: 1.5rem;
      border-radius: 8px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.5);
      margin-bottom: 2rem;
    }
    textarea {
      width: 100%;
      height: 150px;
      background: #28293d;
      color: var(--fg);
      border: 1px solid #444;
      border-radius: 4px;
      padding: 0.5rem;
      font-family: monospace;
      font-size: 0.9rem;
      margin-bottom: 1.5rem;
    }
    .toggles {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 1rem;
      margin-bottom: 1.5rem;
    }
    .toggle-group {
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    input[type="checkbox"] { display: none; }
    label.toggle-btn {
      display: inline-block;
      padding: 0.6rem 1.2rem;
      border: 2px solid var(--accent);
      border-radius: 6px;
      cursor: pointer;
      transition: all 0.2s;
      background: var(--card);
      color: var(--fg);
      text-transform: capitalize;
      box-shadow: 0 0 6px var(--accent);
    }
    input[type="checkbox"]:checked + label.toggle-btn {
      background: var(--accent);
      color: var(--bg);
      box-shadow: 0 0 12px var(--accent);
    }
    .track-group {
      display: flex;
      flex-direction: column;
      align-items: center;
      width: fit-content;
      margin: 0 auto 1.5rem;
      padding: 1rem;
      border: 2px solid var(--accent);
      border-radius: 6px;
      background: var(--accent);
    }
    .track-group label {
      margin-bottom: 0.5rem;
      font-size: 1rem;
      color: var(--bg);
    }
    .track-input {
      background: var(--card);
      border: 2px solid #444;
      border-radius: 6px;
      padding: 0.6rem 1.2rem;
      color: var(--fg);
      width: 100px;
      text-align: center;
      font-size: 1rem;
    }
    .track-input:focus {
      background: var(--accent);
      color: var(--bg);
      box-shadow: 0 0 10px var(--accent);
      outline: none;
    }
    button {
      display: block;
      margin: 2rem auto 0;
      background: var(--accent);
      color: var(--bg);
      border: none;
      padding: 0.8rem 2rem;
      font-size: 1rem;
      border-radius: 6px;
      cursor: pointer;
      box-shadow: 0 0 6px var(--accent);
      transition: all 0.2s;
    }
    button:hover {
      background: var(--card);
      box-shadow: 0 0 12px var(--accent);
    }
    .output, .file-view {
      background: var(--card);
      padding: 1rem;
      border-radius: 6px;
      margin-bottom: 2rem;
      font-family: monospace;
      white-space: pre-wrap;
      color: var(--fg);
      border: 2px solid var(--accent);
    }
    .register-box {
      border: 2px solid var(--accent);
      padding: 1rem;
      border-radius: 10px;
      box-shadow: 0 0 12px var(--accent);
      background: #2e2e42;
      margin-bottom: 2rem;
    }
    .reg-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 1rem;
    }
    .reg-item {
      background: #28293d;
      padding: 0.7rem;
      border-radius: 6px;
      text-align: center;
      font-weight: bold;
      color: var(--fg);
      box-shadow: 0 0 6px var(--accent); /* small glow */
      transition: box-shadow 0.3s;
    }
    .reg-item:hover {
      box-shadow: 0 0 12px var(--accent); /* glow boost on hover */
    }
    h2 {
      color: var(--accent);
      margin: 2rem 0 1rem;
      text-align: center;
    }
  </style>
</head>
<body>
  <header>
    <h1>RISC-V Simulator</h1>
  </header>
  <main>
    <form method="post" action="/run">
      <textarea name="asm_code" placeholder="Paste your assembly here…">{{ asm_code }}</textarea>

      <div class="toggles">
        {% for key, label in knobs %}
          <div class="toggle-group">
            <input type="checkbox" id="{{ key }}" name="{{ key }}" value="yes" {% if form_values[key]=='yes' %}checked{% endif %}>
            <label for="{{ key }}" class="toggle-btn">{{ label }}</label>
          </div>
        {% endfor %}
        <div class="toggle-group">
          <input type="checkbox" id="{{ branch_knob[0] }}" name="{{ branch_knob[0] }}" value="yes" {% if form_values[branch_knob[0]]=='yes' %}checked{% endif %}>
          <label for="{{ branch_knob[0] }}" class="toggle-btn">{{ branch_knob[1] }}</label>
        </div>
      </div>

      <div class="track-group">
        <label for="track_instr">Instruction Track</label>
        <input type="number" id="track_instr" name="track_instr" class="track-input" value="{{ track_instr }}">
      </div>

      <button type="submit">Run Simulation</button>
    </form>

    {% if output is defined %}
      <h2>▶ Console Output</h2>
      <div class="output">{{ output }}</div>

      <h2>Register File</h2>
      <div class="register-box">
        <div class="reg-grid">
          {% for name, val in registers.items() %}
            <div class="reg-item">{{ name }} : {{ val }}</div>
          {% endfor %}
        </div>
      </div>

      <h2>Simulation Stats</h2>
      <div class="file-view">{{ stats }}</div>

      <h2>Text & Data Segment</h2>
      <div class="file-view">{{ output_mc }}</div>

      <h2>Memory Dump</h2>
      <div class="file-view">{{ memory }}</div>
    {% endif %}
  </main>
</body>
</html>
'''

@app.route('/', methods=['GET'])
def index():
    default = {key: 'yes' for key, _ in KNOBS}
    default[BRANCH_KNOB[0]] = 'yes'
    return render_template_string(
        TEMPLATE,
        knobs=KNOBS,
        branch_knob=BRANCH_KNOB,
        asm_code='',
        track_instr='-1',
        form_values=default
    )

@app.route('/run', methods=['POST'])
def run_sim():
    asm = request.form.get('asm_code','')
    with open('input.asm','w') as f:
        f.write(asm)

    form_values = {key: 'yes' if request.form.get(key)=='yes' else 'no' for key,_ in KNOBS}
    form_values[BRANCH_KNOB[0]] = 'yes' if request.form.get(BRANCH_KNOB[0])=='yes' else 'no'
    track = request.form.get('track_instr','-1')

    answers = [form_values[k] for k,_ in KNOBS] + [track, form_values[BRANCH_KNOB[0]]]
    stdin = "\n".join(answers) + "\n"

    sim_exe = 'code.exe' if os.name=='nt' else 'code'
    try:
        proc = subprocess.run([sim_exe], input=stdin,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, check=True)
        output = proc.stdout
    except FileNotFoundError:
        output = f"Error: '{sim_exe}' not found. Compile it with: g++ code.cpp -o {sim_exe}"
    except subprocess.CalledProcessError as e:
        output = f"Simulator error (exit code {e.returncode}):\n{e.stdout}"

    return render_template_string(
        TEMPLATE,
        knobs=KNOBS,
        branch_knob=BRANCH_KNOB,
        asm_code=asm,
        track_instr=track,
        form_values=form_values,
        output=output,
        registers=get_register_file(),
        stats=read_file('stats.txt'),
        output_mc=read_file('output.mc'),
        memory=read_file('memory.txt')
    )

if __name__ == '__main__':
    app.run(debug=True)
