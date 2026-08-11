import os
import subprocess
from flask import Flask, Response, render_template

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/stream-game')
def stream_game():
    def generate():
        # Execute the compiled game binary and capture output stream
        process = subprocess.Popen(
            ['./game'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
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