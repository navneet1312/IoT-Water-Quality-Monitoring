/* ============================================
   AquaGuard Dashboard - Interactive JavaScript
   Real-time simulation with Canvas charts
   ============================================ */

// ====== Configuration ======
const CONFIG = {
    UPDATE_INTERVAL: 2000,
    CHART_POINTS: 30,
    THRESHOLDS: {
        pH: { min: 6.5, max: 8.5 },
        turbidity: { max: 50 },
        tds: { max: 500 },
        temperature: { min: 5, max: 35 },
        humidity: { min: 30, max: 80 }
    }
};

// ====== State ======
const state = {
    data: {
        pH: [],
        turbidity: [],
        tds: [],
        temperature: [],
        humidity: []
    },
    uploadCount: 0,
    alertCount: 0,
    startTime: Date.now(),
    alerts: []
};

// ====== Sensor Simulation ======
function generateSensorData() {
    const time = Date.now() / 1000;
    
    // Simulate realistic sensor values with natural variation
    const pH = 7.0 + Math.sin(time * 0.05) * 1.2 + (Math.random() - 0.5) * 0.4;
    const turbidity = 25 + Math.sin(time * 0.03) * 20 + (Math.random() - 0.5) * 10;
    const tds = 300 + Math.sin(time * 0.04) * 180 + (Math.random() - 0.5) * 40;
    const temperature = 22 + Math.sin(time * 0.02) * 8 + (Math.random() - 0.5) * 2;
    const humidity = 65 + Math.sin(time * 0.025) * 15 + (Math.random() - 0.5) * 5;

    return {
        pH: Math.max(0, Math.min(14, pH)),
        turbidity: Math.max(0, Math.min(100, turbidity)),
        tds: Math.max(0, Math.min(1000, tds)),
        temperature: Math.max(-5, Math.min(50, temperature)),
        humidity: Math.max(0, Math.min(100, humidity))
    };
}

// ====== Water Quality Index Calculator ======
function calculateWQI(data) {
    let score = 100;
    
    // pH scoring (ideal: 7.0, safe: 6.5-8.5)
    const phDeviation = Math.abs(data.pH - 7.0);
    score -= phDeviation * 8;
    
    // Turbidity scoring (lower is better, max safe: 50)
    score -= (data.turbidity / 50) * 15;
    
    // TDS scoring (lower is better, max safe: 500)
    score -= (data.tds / 500) * 15;
    
    // Temperature scoring (ideal: 15-25)
    if (data.temperature < 5 || data.temperature > 35) {
        score -= 15;
    } else if (data.temperature < 10 || data.temperature > 30) {
        score -= 8;
    }
    
    return Math.max(0, Math.min(100, Math.round(score)));
}

function getWQIStatus(wqi) {
    if (wqi >= 90) return { text: 'Excellent Water Quality', color: '#10b981', class: 'excellent' };
    if (wqi >= 70) return { text: 'Good Water Quality', color: '#34d399', class: 'good' };
    if (wqi >= 50) return { text: 'Fair Water Quality', color: '#f59e0b', class: 'fair' };
    if (wqi >= 25) return { text: 'Poor Water Quality', color: '#f97316', class: 'poor' };
    return { text: 'Dangerous - Immediate Action Required!', color: '#ef4444', class: 'danger' };
}

// ====== Check Thresholds & Generate Alerts ======
function checkThresholds(data) {
    const alerts = [];
    const th = CONFIG.THRESHOLDS;
    
    if (data.pH < th.pH.min) alerts.push({ level: 'warning', msg: `pH too low: ${data.pH.toFixed(1)} (min: ${th.pH.min})` });
    if (data.pH > th.pH.max) alerts.push({ level: 'warning', msg: `pH too high: ${data.pH.toFixed(1)} (max: ${th.pH.max})` });
    if (data.turbidity > th.turbidity.max) alerts.push({ level: 'danger', msg: `High turbidity: ${data.turbidity.toFixed(1)} NTU (max: ${th.turbidity.max})` });
    if (data.tds > th.tds.max) alerts.push({ level: 'danger', msg: `High TDS: ${data.tds.toFixed(0)} ppm (max: ${th.tds.max})` });
    if (data.temperature < th.temperature.min) alerts.push({ level: 'warning', msg: `Water too cold: ${data.temperature.toFixed(1)}°C` });
    if (data.temperature > th.temperature.max) alerts.push({ level: 'warning', msg: `Water too hot: ${data.temperature.toFixed(1)}°C` });
    
    return alerts;
}

// ====== Update UI ======
function updateDashboard() {
    const data = generateSensorData();
    const now = new Date();
    
    // Store historical data
    Object.keys(state.data).forEach(key => {
        state.data[key].push(data[key]);
        if (state.data[key].length > CONFIG.CHART_POINTS) {
            state.data[key].shift();
        }
    });

    // Update timestamp
    document.getElementById('lastUpdate').textContent = now.toLocaleTimeString();

    // ====== Update WQI ======
    const wqi = calculateWQI(data);
    const wqiStatus = getWQIStatus(wqi);
    
    document.getElementById('wqiValue').textContent = wqi;
    document.getElementById('wqiStatus').textContent = wqiStatus.text;
    document.getElementById('wqiStatus').style.color = wqiStatus.color;
    
    const dashOffset = 534 - (534 * wqi / 100);
    document.getElementById('wqiRing').style.strokeDashoffset = dashOffset;
    
    // Update WQI gradient based on score
    const wqiGrad = document.getElementById('wqiGrad');
    if (wqi >= 70) {
        wqiGrad.children[0].setAttribute('stop-color', '#00e5ff');
        wqiGrad.children[1].setAttribute('stop-color', '#00c853');
        wqiGrad.children[2].setAttribute('stop-color', '#76ff03');
    } else if (wqi >= 50) {
        wqiGrad.children[0].setAttribute('stop-color', '#f59e0b');
        wqiGrad.children[1].setAttribute('stop-color', '#f97316');
        wqiGrad.children[2].setAttribute('stop-color', '#ef4444');
    } else {
        wqiGrad.children[0].setAttribute('stop-color', '#ef4444');
        wqiGrad.children[1].setAttribute('stop-color', '#dc2626');
        wqiGrad.children[2].setAttribute('stop-color', '#b91c1c');
    }

    const wqiMessage = wqi >= 70 
        ? 'All parameters within acceptable range. Water is safe for designated use.' 
        : wqi >= 50 
        ? 'Some parameters approaching threshold. Monitoring closely recommended.'
        : 'Critical levels detected! Immediate investigation and action required.';
    document.getElementById('wqiMessage').textContent = wqiMessage;

    // ====== Update Sensor Cards ======
    updateSensorCard('ph', data.pH, 14, data.pH.toFixed(1), 
        data.pH >= 6.5 && data.pH <= 8.5);
    updateSensorCard('turb', data.turbidity, 100, data.turbidity.toFixed(1), 
        data.turbidity <= 50);
    updateSensorCard('tds', data.tds, 1000, data.tds.toFixed(0), 
        data.tds <= 500);
    updateSensorCard('temp', data.temperature, 50, data.temperature.toFixed(1), 
        data.temperature >= 5 && data.temperature <= 35);
    updateSensorCard('hum', data.humidity, 100, data.humidity.toFixed(0), 
        data.humidity >= 30 && data.humidity <= 80);

    // ====== Check Thresholds & Alerts ======
    const alerts = checkThresholds(data);
    if (alerts.length > 0) {
        state.alertCount += alerts.length;
        document.getElementById('alertCount').textContent = state.alertCount;
        alerts.forEach(a => addAlert(a.level, a.msg));
    }

    // ====== Cloud Upload Simulation ======
    state.uploadCount++;
    document.getElementById('uploadCount').textContent = state.uploadCount;
    
    const uptime = Math.floor((Date.now() - state.startTime) / 1000);
    const hours = Math.floor(uptime / 3600);
    const mins = Math.floor((uptime % 3600) / 60);
    document.getElementById('uptimeValue').textContent = hours > 0 ? `${hours}h ${mins}m` : `${mins}m`;

    addCloudLog(`[${now.toLocaleTimeString()}] Data sent → pH:${data.pH.toFixed(1)} T:${data.turbidity.toFixed(0)} TDS:${data.tds.toFixed(0)}`);

    // ====== Draw Mini Charts ======
    drawMiniChart('phChart', state.data.pH, '#00d4ff', 0, 14);
    drawMiniChart('turbChart', state.data.turbidity, '#a78bfa', 0, 100);
    drawMiniChart('tdsChart', state.data.tds, '#f59e0b', 0, 1000);
    drawMiniChart('tempChart', state.data.temperature, '#ef4444', -5, 50);
    drawMiniChart('humChart', state.data.humidity, '#22d3ee', 0, 100);

    // ====== Draw Main Charts ======
    drawMainChart();
    drawEnvChart();
}

function updateSensorCard(prefix, value, max, displayValue, isNormal) {
    document.getElementById(`${prefix}Value`).textContent = displayValue;
    document.getElementById(`${prefix}Bar`).style.width = `${(value / max) * 100}%`;
    
    const badge = document.getElementById(`${prefix}Badge`);
    const card = document.getElementById(`${prefix}Card`);
    
    if (isNormal) {
        badge.textContent = 'Normal';
        badge.className = 'sensor-badge';
        card.classList.remove('alert');
    } else {
        badge.textContent = 'Alert!';
        badge.className = 'sensor-badge danger';
        card.classList.add('alert');
    }
}

function addAlert(level, message) {
    const log = document.getElementById('alertLog');
    const entry = document.createElement('div');
    entry.className = `alert-entry ${level}`;
    entry.innerHTML = `
        <span class="alert-time">${new Date().toLocaleTimeString()}</span>
        <span class="alert-msg">${message}</span>
    `;
    log.insertBefore(entry, log.firstChild);
    
    // Keep only last 50 alerts
    while (log.children.length > 50) {
        log.removeChild(log.lastChild);
    }
}

function addCloudLog(message) {
    const log = document.getElementById('cloudLog');
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.textContent = message;
    log.insertBefore(entry, log.firstChild);
    
    while (log.children.length > 20) {
        log.removeChild(log.lastChild);
    }
}

// ====== Mini Sparkline Charts ======
function drawMiniChart(containerId, data, color, min, max) {
    const container = document.getElementById(containerId);
    if (!container || data.length < 2) return;

    let canvas = container.querySelector('canvas');
    if (!canvas) {
        canvas = document.createElement('canvas');
        canvas.width = container.clientWidth || 250;
        canvas.height = 50;
        container.appendChild(canvas);
    }

    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    
    ctx.clearRect(0, 0, w, h);

    const points = data.slice(-CONFIG.CHART_POINTS);
    const stepX = w / (points.length - 1);

    // Draw area
    ctx.beginPath();
    ctx.moveTo(0, h);
    points.forEach((val, i) => {
        const x = i * stepX;
        const y = h - ((val - min) / (max - min)) * h;
        if (i === 0) ctx.lineTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.lineTo(w, h);
    ctx.closePath();

    const gradient = ctx.createLinearGradient(0, 0, 0, h);
    gradient.addColorStop(0, color + '30');
    gradient.addColorStop(1, color + '05');
    ctx.fillStyle = gradient;
    ctx.fill();

    // Draw line
    ctx.beginPath();
    points.forEach((val, i) => {
        const x = i * stepX;
        const y = h - ((val - min) / (max - min)) * h;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    });
    ctx.strokeStyle = color;
    ctx.lineWidth = 2;
    ctx.stroke();

    // Draw last point dot
    const lastX = (points.length - 1) * stepX;
    const lastY = h - ((points[points.length - 1] - min) / (max - min)) * h;
    ctx.beginPath();
    ctx.arc(lastX, lastY, 3, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.fill();
    ctx.beginPath();
    ctx.arc(lastX, lastY, 5, 0, Math.PI * 2);
    ctx.strokeStyle = color;
    ctx.lineWidth = 1;
    ctx.globalAlpha = 0.4;
    ctx.stroke();
    ctx.globalAlpha = 1;
}

// ====== Main Chart ======
function drawMainChart() {
    const canvas = document.getElementById('mainChart');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    
    ctx.clearRect(0, 0, w, h);

    // Grid lines
    ctx.strokeStyle = 'rgba(255,255,255,0.05)';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 5; i++) {
        const y = (h / 5) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }

    const datasets = [
        { data: state.data.pH, color: '#00d4ff', label: 'pH', scale: { min: 0, max: 14 } },
        { data: state.data.turbidity, color: '#a78bfa', label: 'Turbidity', scale: { min: 0, max: 100 } },
        { data: state.data.tds, color: '#f59e0b', label: 'TDS', scale: { min: 0, max: 1000 } }
    ];

    // Legend
    ctx.font = '11px Inter';
    let legendX = 10;
    datasets.forEach(ds => {
        ctx.fillStyle = ds.color;
        ctx.fillRect(legendX, 10, 12, 3);
        ctx.fillStyle = '#94a3b8';
        ctx.fillText(ds.label, legendX + 18, 14);
        legendX += ctx.measureText(ds.label).width + 40;
    });

    // Draw each dataset
    datasets.forEach(ds => {
        if (ds.data.length < 2) return;
        const points = ds.data.slice(-CONFIG.CHART_POINTS);
        const stepX = w / (points.length - 1);
        const padding = 30;
        const chartH = h - padding;

        // Area fill
        ctx.beginPath();
        ctx.moveTo(0, h);
        points.forEach((val, i) => {
            const x = i * stepX;
            const normalized = (val - ds.scale.min) / (ds.scale.max - ds.scale.min);
            const y = chartH - normalized * (chartH - padding);
            ctx.lineTo(x, y);
        });
        ctx.lineTo(w, h);
        ctx.closePath();
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, ds.color + '15');
        grad.addColorStop(1, ds.color + '02');
        ctx.fillStyle = grad;
        ctx.fill();

        // Line
        ctx.beginPath();
        points.forEach((val, i) => {
            const x = i * stepX;
            const normalized = (val - ds.scale.min) / (ds.scale.max - ds.scale.min);
            const y = chartH - normalized * (chartH - padding);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });
        ctx.strokeStyle = ds.color;
        ctx.lineWidth = 2;
        ctx.stroke();
    });
}

// ====== Environment Chart ======
function drawEnvChart() {
    const canvas = document.getElementById('envChart');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width;
    const h = canvas.height;
    
    ctx.clearRect(0, 0, w, h);

    // Grid
    ctx.strokeStyle = 'rgba(255,255,255,0.05)';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 4; i++) {
        const y = (h / 4) * i;
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }

    const datasets = [
        { data: state.data.temperature, color: '#ef4444', label: 'Temperature (°C)', scale: { min: -5, max: 50 } },
        { data: state.data.humidity, color: '#22d3ee', label: 'Humidity (%)', scale: { min: 0, max: 100 } }
    ];

    // Legend
    ctx.font = '11px Inter';
    let legendX = 10;
    datasets.forEach(ds => {
        ctx.fillStyle = ds.color;
        ctx.fillRect(legendX, 10, 12, 3);
        ctx.fillStyle = '#94a3b8';
        ctx.fillText(ds.label, legendX + 18, 14);
        legendX += ctx.measureText(ds.label).width + 40;
    });

    datasets.forEach(ds => {
        if (ds.data.length < 2) return;
        const points = ds.data.slice(-CONFIG.CHART_POINTS);
        const stepX = w / (points.length - 1);
        const padding = 30;
        const chartH = h - padding;

        ctx.beginPath();
        ctx.moveTo(0, h);
        points.forEach((val, i) => {
            const x = i * stepX;
            const normalized = (val - ds.scale.min) / (ds.scale.max - ds.scale.min);
            const y = chartH - normalized * (chartH - padding);
            ctx.lineTo(x, y);
        });
        ctx.lineTo(w, h);
        ctx.closePath();
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, ds.color + '15');
        grad.addColorStop(1, ds.color + '02');
        ctx.fillStyle = grad;
        ctx.fill();

        ctx.beginPath();
        points.forEach((val, i) => {
            const x = i * stepX;
            const normalized = (val - ds.scale.min) / (ds.scale.max - ds.scale.min);
            const y = chartH - normalized * (chartH - padding);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });
        ctx.strokeStyle = ds.color;
        ctx.lineWidth = 2;
        ctx.stroke();

        // Last point
        const lastVal = points[points.length - 1];
        const lastX = (points.length - 1) * stepX;
        const lastNorm = (lastVal - ds.scale.min) / (ds.scale.max - ds.scale.min);
        const lastY = chartH - lastNorm * (chartH - padding);
        ctx.beginPath();
        ctx.arc(lastX, lastY, 4, 0, Math.PI * 2);
        ctx.fillStyle = ds.color;
        ctx.fill();
    });
}

// ====== Initialize ======
function init() {
    console.log('AquaGuard Dashboard initialized');
    addAlert('success', 'System initialized successfully. Sensors online.');
    addCloudLog('[System] Connected to ThingSpeak cloud platform');
    
    // Initial data populate
    for (let i = 0; i < 10; i++) {
        const data = generateSensorData();
        Object.keys(state.data).forEach(key => {
            state.data[key].push(data[key]);
        });
    }

    updateDashboard();
    setInterval(updateDashboard, CONFIG.UPDATE_INTERVAL);
}

// Start when DOM is ready
document.addEventListener('DOMContentLoaded', init);
