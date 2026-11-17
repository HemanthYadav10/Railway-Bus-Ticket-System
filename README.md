# Railway / Bus Ticket Waiting List Management System

This project demonstrates a queue-based waiting list management system for railway or bus tickets using the C programming language and WebAssembly (WASM) with Emscripten.

## Features

- Queue implementation written in C
- WebAssembly integration using Emscripten
- Modern responsive web dashboard
- Passenger registration and waiting list generation
- FIFO queue visualization
- Dequeue, peek, search, and cancel operations
- Attractive dashboard with live statistics
- Railway and bus transport selection

## Project Files

- `queue.c` — C queue implementation
- `index.html` — app layout
- `style.css` — styling and responsive design
- `script.js` — UI logic and WebAssembly interop
- `queue.js` and `queue.wasm` — generated Wasm build files

## Required Tools

- Python 3
- Node.js
- Emscripten SDK

## Setup

### 1) Install Node.js

Download and install Node.js LTS from:
https://nodejs.org/

### 2) Install Emscripten SDK

Open a terminal and run:

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
```

On Windows PowerShell, use:

```powershell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
```

Then activate the environment for the current shell:

```powershell
.\emsdk_env.bat
```

### 3) Compile the C Queue to WebAssembly

From the project root, run:

```bash
emcc queue.c -O2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME='createQueueModule' -s EXPORTED_FUNCTIONS="['_queue_reset','_queue_enqueue','_queue_dequeue_json','_queue_peek_json','_queue_get_all_records_json','_queue_get_queue_json','_queue_get_dashboard_json','_queue_empty_status','_queue_full_status','_queue_search_json','_queue_cancel_by_booking_json']" -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']" -o queue.js
```

### 4) Run the Project Locally

In the project folder, run:

```bash
python -m http.server 8000
```

Then open:

```text
http://localhost:8000
```

## Queue Operations

The C queue supports:

- Enqueue
- Dequeue
- Peek
- Display
- Is Empty
- Is Full
- Search
- Cancel registration

## Project Objective

This system demonstrates the use of the queue data structure in ticket reservation systems. It follows the FIFO principle and is useful for real-world railway and bus waiting list management.

## Author

College project submission for Queue Data Structure demonstration.
