import os
import subprocess
from flask import Flask, Response, render_template

# Ensure Flask looks in the exact directory relative to app.py
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
app = Flask(__name__, template_folder=os.path.join(BASE_DIR, 'templates'))

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/stream-game')
def stream_game():
    def generate():
        # Execute the compiled game binary from absolute working directory
        game_path = os.path.join(BASE_DIR, 'game')
        
        process = subprocess.Popen(
            [game_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            cwd=BASE_DIR  # Ensures child processes like barbarian/rogue are found in working dir
        )
        
        # Stream live stdout lines to the browser
        for line in iter(process.stdout.readline, ''):
            yield f"data: {line}\n\n"
            
        process.stdout.close()
        process.wait()

    return Response(generate(), mimetype='text/event-stream')

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port)
