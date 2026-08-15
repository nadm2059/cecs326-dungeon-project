import os
import re
import subprocess
from flask import Flask, Response, render_template

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
app = Flask(__name__, template_folder=os.path.join(BASE_DIR, 'templates'))

# Regex to strip ANSI escape sequences (\x1b[...m)
ANSI_ESCAPE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/stream-game')
def stream_game():
    def generate():
        game_path = os.path.join(BASE_DIR, 'game')
        
        process = subprocess.Popen(
            ['stdbuf', '-oL', '-eL', game_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd=BASE_DIR
        )
        
        for line in iter(process.stdout.readline, ''):
            clean_line = ANSI_ESCAPE.sub('', line).rstrip()
            if clean_line:
                yield f"data: {clean_line}\n\n"
            
        process.stdout.close()
        process.wait()

        # Send an explicit completion signal to the frontend
        yield "data: [FINISHED]\n\n"

    return Response(generate(), mimetype='text/event-stream')

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port)
