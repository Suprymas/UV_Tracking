FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

WORKDIR /app

# Copy requirements and install dependencies
COPY requirements.txt /app/
RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

# Copy the websocket_server module files
COPY . /app/websocket_server/

# Set PYTHONPATH to include /app
ENV PYTHONPATH=/app

# Expose WebSocket port
EXPOSE 8765

# Run the server
WORKDIR /app
CMD ["python", "-m", "websocket_server.main"]

