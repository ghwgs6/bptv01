// ApexYAML Studio - Client Application Logic with Drag & Drop

const codeEditor = document.getElementById('codeEditor');
const lineNumbers = document.getElementById('lineNumbers');
const fileSelect = document.getElementById('fileSelect');
const btnSave = document.getElementById('btnSave');
const btnValidate = document.getElementById('btnValidate');
const btnESPHomeConfig = document.getElementById('btnESPHomeConfig');
const btnFlash = document.getElementById('btnFlash');
const btnFormat = document.getElementById('btnFormat');
const btnCopy = document.getElementById('btnCopy');
const btnDownload = document.getElementById('btnDownload');
const dropOverlay = document.getElementById('dropOverlay');

const statusCard = document.getElementById('statusCard');
const statusTitle = document.getElementById('statusTitle');
const statusMsg = document.getElementById('statusMsg');

const statLines = document.getElementById('statLines');
const statChars = document.getElementById('statChars');
const statCursor = document.getElementById('statCursor');
const inspectorContent = document.getElementById('inspectorContent');
const toastContainer = document.getElementById('toastContainer');

// Snippet Buttons
const snippetButtons = document.querySelectorAll('.snippet-btn[draggable="true"]');

const snippets = {
  snipESPNowSender: `
# Board 1 ESP-NOW Sender Initialization
includes:
  - esp_now_sender.h
on_boot:
  priority: 800.0
  then:
    - lambda: |-
        global_esp_now_sender = new ESPNowSenderComponent();
        global_esp_now_sender->setup();`,

  snipESPNowReceiver: `
# Board 2 ESP-NOW Receiver Initialization
includes:
  - esp_now_receiver.h
on_boot:
  priority: 800.0
  then:
    - lambda: |-
        static auto *my_receiver = new ESPNowReceiverComponent();
        my_receiver->setup();`,

  snipDualWiFi: `
wifi:
  networks:
    - ssid: "H824"
      password: "gw202001"
      priority: 10.0
    - ssid: "Pulse_Staff"
      password: "Pulse@1234"
      priority: 5.0
  fast_connect: true`,

  snipMQTTBroker: `
mqtt:
  broker: 192.168.1.128
  port: 1883
  topic_prefix: farm/inputs`,

  snipDigitalInput: `
  - platform: gpio
    pin: { number: GPIO4, mode: INPUT_PULLUP, inverted: true }
    name: "DI 01 | Pin L05 (D4 / GPIO04)"
    id: di_01
    on_state:
      then:
        - lambda: |-
            send_esp_now_packet(x ? "DI_01:ON" : "DI_01:OFF");`,

  snipWebServer: `
web_server:
  port: 80
  version: 2
  auth:
    username: "admin"
    password: "password123"`
};

let currentFilePath = fileSelect.value;
let draggedSnippetId = null;

// Initialize Editor
window.addEventListener('DOMContentLoaded', () => {
  loadFile(fileSelect.value);
  updateLineNumbers();
  validateYAML();
  setupDragAndDrop();
});

// Update Line Numbers & Cursor Stats
function updateLineNumbers() {
  const text = codeEditor.value;
  const lines = text.split('\n');
  const count = lines.length;

  let lineNumHTML = '';
  for (let i = 1; i <= count; i++) {
    lineNumHTML += i + '<br>';
  }
  lineNumbers.innerHTML = lineNumHTML;

  // Stats
  statLines.textContent = `Lines: ${count}`;
  statChars.textContent = `Chars: ${text.length}`;

  updateCursorPos();
}

function updateCursorPos() {
  const text = codeEditor.value;
  const selStart = codeEditor.selectionStart;
  const lines = text.substring(0, selStart).split('\n');
  const lineNum = lines.length;
  const colNum = lines[lines.length - 1].length + 1;

  statCursor.textContent = `Ln ${lineNum}, Col ${colNum}`;
}

// Sync Scroll
codeEditor.addEventListener('scroll', () => {
  lineNumbers.scrollTop = codeEditor.scrollTop;
});

// Editor Input Listener
codeEditor.addEventListener('input', () => {
  updateLineNumbers();
  validateYAML();
});

codeEditor.addEventListener('keyup', updateCursorPos);
codeEditor.addEventListener('click', updateCursorPos);

// Tab & Enter Key Formatting Controls
codeEditor.addEventListener('keydown', (e) => {
  if (e.key === 'Tab') {
    e.preventDefault();
    const start = codeEditor.selectionStart;
    const end = codeEditor.selectionEnd;
    const spaces = '  ';

    codeEditor.value = codeEditor.value.substring(0, start) + spaces + codeEditor.value.substring(end);
    codeEditor.selectionStart = codeEditor.selectionEnd = start + 2;
    updateLineNumbers();
    validateYAML();
  }

  // Save Shortcut: Ctrl+S or Cmd+S
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 's') {
    e.preventDefault();
    saveFile();
  }
});

// Validate YAML Syntax with js-yaml
function validateYAML() {
  const content = codeEditor.value;
  if (!content.trim()) {
    setStatus('valid', 'Empty Document', 'No YAML content to parse.');
    return true;
  }

  try {
    const doc = jsyaml.load(content);
    setStatus('valid', 'YAML Valid', 'Syntax is clean and well-formed.');

    if (typeof doc === 'object' && doc !== null) {
      inspectorContent.textContent = '--- Parsed YAML Structure ---\n\n' + JSON.stringify(doc, null, 2);
    }
    return true;
  } catch (err) {
    const errLine = err.mark ? `Line ${err.mark.line + 1}, Col ${err.mark.column + 1}` : 'Syntax Error';
    setStatus('invalid', 'YAML Syntax Error', `${errLine}: ${err.reason || err.message}`);
    return false;
  }
}

function setStatus(type, title, msg) {
  statusCard.className = `status-card ${type}`;
  statusTitle.textContent = title;
  statusMsg.textContent = msg;
}

// Toast Notifications
function showToast(message, type = 'info') {
  const toast = document.createElement('div');
  toast.className = 'toast';
  toast.innerHTML = `<span>${type === 'success' ? '✅' : type === 'error' ? '❌' : 'ℹ️'}</span> ${message}`;
  toastContainer.appendChild(toast);

  setTimeout(() => {
    toast.remove();
  }, 3500);
}

// Insert Snippet at Cursor Position
function insertSnippet(snippetText) {
  const start = codeEditor.selectionStart;
  const end = codeEditor.selectionEnd;
  const currentText = codeEditor.value;

  codeEditor.value = currentText.substring(0, start) + snippetText + currentText.substring(end);
  codeEditor.selectionStart = codeEditor.selectionEnd = start + snippetText.length;
  codeEditor.focus();

  updateLineNumbers();
  validateYAML();
  showToast('Snippet inserted!', 'success');
}

// DRAG & DROP FUNCTIONALITY
function setupDragAndDrop() {
  // 1. File Drag & Drop onto Window
  ['dragenter', 'dragover'].forEach(eventName => {
    window.addEventListener(eventName, (e) => {
      e.preventDefault();
      e.stopPropagation();
      if (e.dataTransfer.types.includes('Files')) {
        dropOverlay.classList.add('active');
      }
    }, false);
  });

  ['dragleave', 'drop'].forEach(eventName => {
    window.addEventListener(eventName, (e) => {
      e.preventDefault();
      e.stopPropagation();
      dropOverlay.classList.remove('active');
    }, false);
  });

  window.addEventListener('drop', (e) => {
    const files = e.dataTransfer.files;
    if (files.length > 0) {
      const file = files[0];
      if (file.name.endsWith('.yaml') || file.name.endsWith('.yml') || file.name.endsWith('.txt')) {
        const reader = new FileReader();
        reader.onload = (event) => {
          codeEditor.value = event.target.result;
          currentFilePath = file.name;
          fileSelect.value = 'custom';
          updateLineNumbers();
          validateYAML();
          showToast(`Loaded ${file.name} via Drag & Drop!`, 'success');
        };
        reader.readAsText(file);
      } else {
        showToast('Please drop a valid .yaml file', 'error');
      }
    }
  });

  // 2. Snippet Drag & Drop into Code Editor
  snippetButtons.forEach(btn => {
    btn.addEventListener('dragstart', (e) => {
      draggedSnippetId = btn.id;
      e.dataTransfer.setData('text/plain', snippets[btn.id] || '');
    });

    btn.addEventListener('click', () => {
      if (snippets[btn.id]) {
        insertSnippet(snippets[btn.id]);
      }
    });
  });

  codeEditor.addEventListener('dragover', (e) => {
    e.preventDefault();
  });

  codeEditor.addEventListener('drop', (e) => {
    e.preventDefault();
    const snippetContent = e.dataTransfer.getData('text/plain');
    if (snippetContent) {
      insertSnippet(snippetContent);
    }
  });
}

// Quick Action Event Listeners
btnValidate.addEventListener('click', () => {
  if (validateYAML()) {
    showToast('YAML validation passed!', 'success');
  } else {
    showToast('YAML validation failed. Check line errors.', 'error');
  }
});

btnCopy.addEventListener('click', () => {
  navigator.clipboard.writeText(codeEditor.value);
  showToast('YAML copied to clipboard!', 'success');
});

btnDownload.addEventListener('click', () => {
  const blob = new Blob([codeEditor.value], { type: 'text/yaml' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = fileSelect.value === 'custom' ? 'esphome.yaml' : fileSelect.value.split('/').pop();
  a.click();
  URL.revokeObjectURL(url);
  showToast('File downloaded!', 'success');
});

fileSelect.addEventListener('change', (e) => {
  loadFile(e.target.value);
});

// File I/O API Calls to Local Backend
async function loadFile(filename) {
  if (filename === 'custom') {
    codeEditor.value = '# New ESPHome Configuration Document\nesphome:\n  name: my-esp32-project\n\nesp32:\n  board: esp32dev\n\nlogger:\n';
    updateLineNumbers();
    validateYAML();
    return;
  }

  currentFilePath = filename;

  try {
    const res = await fetch(`/api/load-file?path=${encodeURIComponent(filename)}`);
    const data = await res.json();

    if (data.success) {
      codeEditor.value = data.content;
      updateLineNumbers();
      validateYAML();
      showToast(`Loaded ${filename.split('/').pop()}`, 'success');
    } else {
      showToast(`Could not load file: ${data.error}`, 'error');
    }
  } catch (err) {
    console.warn('Backend server response warning', err);
  }
}

async function saveFile() {
  const content = codeEditor.value;

  try {
    const res = await fetch('/api/save-file', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ path: currentFilePath, content })
    });
    const data = await res.json();

    if (data.success) {
      showToast(`Saved to ${currentFilePath.split('/').pop()}`, 'success');
    } else {
      showToast(`Save failed: ${data.error}`, 'error');
    }
  } catch (err) {
    showToast('Backend server unavailable.', 'error');
  }
}

btnSave.addEventListener('click', saveFile);

// ESPHome Terminal Validation Command
btnESPHomeConfig.addEventListener('click', async () => {
  inspectorContent.textContent = '⏳ Running `/Users/gerhardwillemse/.local/bin/esphome config` validation...\n\nPlease wait...';
  showToast('Running ESPHome validation check...', 'info');

  try {
    await saveFile();

    const res = await fetch(`/api/esphome-check?path=${encodeURIComponent(currentFilePath)}`);
    const data = await res.json();

    if (data.success) {
      inspectorContent.textContent = '✅ ESPHome Validation Output:\n\n' + data.output;
      showToast('ESPHome Validation Passed!', 'success');
    } else {
      inspectorContent.textContent = '❌ ESPHome Errors Detected:\n\n' + (data.output || data.error);
      showToast('ESPHome Check Failed!', 'error');
    }
  } catch (err) {
    inspectorContent.textContent = 'Error executing ESPHome CLI check: ' + err.message;
  }
});

// Flash directly to connected ESP32
btnFlash.addEventListener('click', async () => {
  inspectorContent.textContent = '⚡ Initiating ESPHome compilation and flash upload...\n\nPlease wait while binaries are compiled and written over USB serial...';
  showToast('Compiling & Flashing ESP32...', 'info');

  try {
    await saveFile();

    const res = await fetch('/api/flash-esp32', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ path: currentFilePath })
    });
    const data = await res.json();

    if (data.success) {
      inspectorContent.textContent = '🎉 FLASH SUCCESSFUL!\n\n' + data.output;
      showToast('Flash Completed Successfully!', 'success');
    } else {
      inspectorContent.textContent = '❌ Flash Output / Log:\n\n' + (data.output || data.error);
      showToast('Flash Process Output logged below.', 'info');
    }
  } catch (err) {
    inspectorContent.textContent = 'Error triggering flash process: ' + err.message;
  }
});
