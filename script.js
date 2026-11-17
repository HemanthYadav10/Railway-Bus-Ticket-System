import createQueueModule from './queue.js';

const app = {
  api: null,
  lastAction: null,
};

const toast = document.getElementById('toast');

function showToast(message, isError = false) {
  toast.textContent = message;
  toast.classList.toggle('error', isError);
  toast.classList.add('visible');

  window.clearTimeout(showToast.timeoutId);
  showToast.timeoutId = window.setTimeout(() => {
    toast.classList.remove('visible');
  }, 2800);
}

function parseJson(value) {
  if (!value || value === 'undefined' || value === 'null') {
    return null;
  }

  try {
    return JSON.parse(value);
  } catch (error) {
    return null;
  }
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) {
    el.textContent = value;
  }
}

function safeString(value) {
  return value == null ? '' : String(value);
}

async function initQueueModule() {
  const module = await createQueueModule();

  app.api = {
    reset: module.cwrap('queue_reset', null, []),
    enqueue: module.cwrap('queue_enqueue', 'number', ['string', 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'number', 'number']),
    dequeue: module.cwrap('queue_dequeue_json', 'string', []),
    peek: module.cwrap('queue_peek_json', 'string', []),
    list: module.cwrap('queue_get_all_records_json', 'string', []),
    queue: module.cwrap('queue_get_queue_json', 'string', []),
    dashboard: module.cwrap('queue_get_dashboard_json', 'string', []),
    empty: module.cwrap('queue_empty_status', 'number', []),
    full: module.cwrap('queue_full_status', 'number', []),
    search: module.cwrap('queue_search_json', 'string', ['string']),
    cancel: module.cwrap('queue_cancel_by_booking_json', 'string', ['string']),
  };
}

function updateDashboard() {
  if (!app.api) {
    return;
  }

  const dashboardData = parseJson(app.api.dashboard());
  if (!dashboardData) {
    return;
  }

  setText('total-passengers', safeString(dashboardData.totalPassengers));
  setText('waiting-passengers', safeString(dashboardData.waitingPassengers));
  setText('served-passengers', safeString(dashboardData.servedPassengers));
  setText('available-capacity', safeString(dashboardData.availableCapacity));
  setText('queue-status', safeString(dashboardData.queueStatus));
  setText('hero-waiting-count', safeString(dashboardData.waitingPassengers));
  setText('hero-served-count', safeString(dashboardData.servedPassengers));
  setText('live-queue-status', safeString(dashboardData.queueStatus));

  const frontPassenger = getFrontPassenger();
  setText('front-passenger-name', frontPassenger ? frontPassenger.name : 'Waiting for passengers');

  const rearPassenger = getRearPassenger();
  setText('rear-passenger-name', rearPassenger ? rearPassenger.name : 'No passengers');
}

function getQueueRows() {
  const raw = app.api.list();
  const parsed = parseJson(raw);
  if (!Array.isArray(parsed)) {
    return [];
  }
  return parsed;
}

function getFrontPassenger() {
  const queueList = parseJson(app.api.queue());
  if (!Array.isArray(queueList) || queueList.length === 0) {
    return null;
  }
  return queueList[0];
}

function getRearPassenger() {
  const queueList = parseJson(app.api.queue());
  if (!Array.isArray(queueList) || queueList.length === 0) {
    return null;
  }
  return queueList[queueList.length - 1];
}

function renderTable() {
  const rows = getQueueRows();
  const tableBody = document.getElementById('waiting-table-body');

  if (!tableBody) {
    return;
  }

  if (!rows.length) {
    tableBody.innerHTML = `
      <tr>
        <td colspan="8" style="text-align:center; color: var(--muted); padding: 24px;">No passengers in the queue.</td>
      </tr>
    `;
    return;
  }

  tableBody.innerHTML = rows
    .map((row) => {
      const statusText = safeString(row.status).toLowerCase();
      const statusClass = statusText === 'served' ? 'status-served' : statusText === 'cancelled' ? 'status-cancelled' : 'status-waiting';
      const route = safeString(row.route || '').replace(/-/g, '→');
      const actionMarkup = statusText === 'waiting'
        ? `
          <button type="button" class="action-chip serve-btn" data-booking-id="${safeString(row.bookingId)}">Serve</button>
          <button type="button" class="action-chip cancel-btn" data-booking-id="${safeString(row.bookingId)}">Cancel</button>
        `
        : '<span style="color: var(--muted);">—</span>';

      return `
        <tr>
          <td>#${safeString(row.waitingNo)}</td>
          <td>${safeString(row.name)}</td>
          <td>${safeString(row.bookingId)}</td>
          <td>${safeString(row.transportType)}</td>
          <td>${route}</td>
          <td>${safeString(row.journeyDate)}</td>
          <td><span class="status-pill ${statusClass}">${safeString(row.status)}</span></td>
          <td>${actionMarkup}</td>
        </tr>
      `;
    })
    .join('');

  const serveButtons = document.querySelectorAll('.serve-btn');
  serveButtons.forEach((button) => {
    button.addEventListener('click', () => {
      const result = parseJson(app.api.dequeue());
      if (result && result.success) {
        showToast(`Served: ${result.passenger.name}`);
      } else {
        showToast(result ? result.message : 'Unable to serve passenger.', true);
      }
      refreshAll();
    });
  });

  const cancelButtons = document.querySelectorAll('.cancel-btn');
  cancelButtons.forEach((button) => {
    button.addEventListener('click', () => {
      const bookingId = button.dataset.bookingId;
      const result = parseJson(app.api.cancel(bookingId));
      if (result && result.success) {
        showToast(`Cancelled: ${result.passenger.bookingId}`);
      } else {
        showToast(result ? result.message : 'Unable to cancel passenger.', true);
      }
      refreshAll();
    });
  });
}

function renderQueueVisualization() {
  const queueNodes = parseJson(app.api.queue());
  const container = document.getElementById('queue-visual-box');

  if (!container) {
    return;
  }

  if (!Array.isArray(queueNodes) || queueNodes.length === 0) {
    container.innerHTML = '<div class="empty-state">Queue is empty.</div>';
    return;
  }

  const pointerFront = '<div class="queue-pointer">FRONT</div>';
  const pointerRear = '<div class="queue-pointer rear">REAR</div>';

  const nodes = queueNodes
    .map((item, index) => {
      const isFront = index === 0;
      return `
        <div class="queue-node ${isFront ? 'highlight' : ''}">
          <span class="queue-badge">#${safeString(item.waitingNo)}</span>
          <p>${safeString(item.name)}</p>
        </div>
      `;
    })
    .join('');

  container.innerHTML = `${pointerFront}${nodes}${pointerRear}`;
}

function refreshAll() {
  updateDashboard();
  renderTable();
  renderQueueVisualization();
}

function validateFormData() {
  const form = document.getElementById('passenger-form');
  const formData = new FormData(form);
  const requiredFields = [
    'fullName',
    'bookingId',
    'age',
    'gender',
    'transportType',
    'source',
    'destination',
    'journeyDate',
    'departureTime',
    'tickets',
  ];

  for (const field of requiredFields) {
    const value = (formData.get(field) || '').toString().trim();
    if (!value) {
      return { ok: false, message: 'Please fill in all required passenger details.' };
    }
  }

  return { ok: true };
}

function handleRegistration(event) {
  event.preventDefault();
  const validation = validateFormData();
  if (!validation.ok) {
    showToast(validation.message, true);
    return;
  }

  const form = event.target;
  const data = new FormData(form);
  const waitingNumber = app.api.enqueue(
    safeString(data.get('fullName')),
    safeString(data.get('bookingId')),
    safeString(data.get('gender')),
    safeString(data.get('transportType')),
    safeString(data.get('source')),
    safeString(data.get('destination')),
    safeString(data.get('journeyDate')),
    safeString(data.get('departureTime')),
    Number(data.get('age')),
    Number(data.get('tickets'))
  );

  if (waitingNumber < 0) {
    showToast('Queue is full. Please serve a passenger before adding more.', true);
    return;
  }

  showToast(`Registration successful! Waiting position: #${waitingNumber}`);
  form.reset();
  refreshAll();
}

function handleControlAction(action) {
  if (!app.api) {
    return;
  }

  switch (action) {
    case 'dequeue': {
      const result = parseJson(app.api.dequeue());
      if (result && result.success) {
        showToast(`Passenger served: ${result.passenger.name}`);
      } else {
        showToast(result ? result.message : 'Queue is empty.', true);
      }
      break;
    }
    case 'peek': {
      const result = parseJson(app.api.peek());
      if (result && result.success) {
        showToast(`Front passenger: ${result.passenger.name}`);
      } else {
        showToast('Queue is empty.', true);
      }
      break;
    }
    case 'display': {
      renderQueueVisualization();
      showToast('Queue refreshed and displayed.');
      break;
    }
    case 'empty': {
      const isEmpty = Number(app.api.empty()) === 1;
      showToast(isEmpty ? 'Queue is empty.' : 'Queue is not empty.');
      break;
    }
    case 'full': {
      const isFull = Number(app.api.full()) === 1;
      showToast(isFull ? 'Queue is full.' : 'Queue is not full.');
      break;
    }
    case 'search': {
      const query = document.getElementById('search-input').value.trim();
      if (!query) {
        showToast('Enter a name or booking ID to search.', true);
        return;
      }
      const result = parseJson(app.api.search(query));
      if (result && result.success) {
        showToast(`Found passenger: ${result.passenger.name}`);
      } else {
        showToast(result ? result.message : 'Passenger not found.', true);
      }
      break;
    }
    case 'cancel': {
      const requestId = document.getElementById('cancel-input').value.trim();
      if (!requestId) {
        showToast('Enter a booking ID to cancel.', true);
        return;
      }
      const result = parseJson(app.api.cancel(requestId));
      if (result && result.success) {
        showToast(`Cancelled booking: ${requestId}`);
      } else {
        showToast(result ? result.message : 'Booking not found.', true);
      }
      break;
    }
    case 'reset': {
      const confirmed = window.confirm('Reset the queue and clear all passengers?');
      if (!confirmed) {
        return;
      }
      app.api.reset();
      showToast('Queue reset successfully.');
      break;
    }
    default:
      break;
  }

  refreshAll();
}

function applyInitialData() {
  app.api.reset();

  const demoPassengers = [
    {
      name: 'Aisha Rahman',
      bookingId: 'RWL-101',
      gender: 'Female',
      transportType: 'Railway',
      source: 'Dhaka',
      destination: 'Chittagong',
      journeyDate: '2026-11-20',
      departureTime: '08:30',
      age: 28,
      tickets: 2,
    },
    {
      name: 'Nabil Hasan',
      bookingId: 'BUS-204',
      gender: 'Male',
      transportType: 'Bus',
      source: 'Sylhet',
      destination: 'Dhaka',
      journeyDate: '2026-11-22',
      departureTime: '12:15',
      age: 35,
      tickets: 1,
    },
    {
      name: 'Mehnaz Akter',
      bookingId: 'RWL-119',
      gender: 'Female',
      transportType: 'Railway',
      source: 'Khulna',
      destination: 'Rajshahi',
      journeyDate: '2026-11-25',
      departureTime: '17:00',
      age: 24,
      tickets: 1,
    },
  ];

  demoPassengers.forEach((passenger) => {
    app.api.enqueue(
      passenger.name,
      passenger.bookingId,
      passenger.gender,
      passenger.transportType,
      passenger.source,
      passenger.destination,
      passenger.journeyDate,
      passenger.departureTime,
      passenger.age,
      passenger.tickets
    );
  });
}

function bindEvents() {
  document.getElementById('passenger-form').addEventListener('submit', handleRegistration);
  document.getElementById('clear-form-btn').addEventListener('click', () => document.getElementById('passenger-form').reset());
  document.getElementById('reset-queue-btn').addEventListener('click', () => handleControlAction('reset'));

  document.querySelectorAll('.control-btn').forEach((button) => {
    button.addEventListener('click', () => handleControlAction(button.dataset.action));
  });
}

async function init() {
  await initQueueModule();
  applyInitialData();
  bindEvents();
  refreshAll();
  showToast('Queue system initialized successfully.');
}

init();
