import Chart from 'chart.js/auto';
import { createWidgetCard, getValueAtPath, mergeWidgetConfig } from './widget-base.js';

const DEFAULTS = {
  apiUrl: '',
  title: 'Chart',
  label: 'Chart',
  icon: 'activity',
  color: 'primary',
  chartType: 'line',
  ranges: ['Last 24 hours', 'Last 7 days', 'Last 30 days'],
  rangeParam: 'range',
  labelsPath: 'labels',
  datasetsPath: 'datasets',
  pollIntervalMs: 0,
};

const SERIES_COLORS = ['#38d5d1', '#a78bfa', '#f59e0b', '#fb7185'];
const DEMO_DATA = {
  'Last 24 hours': {
    labels: ['00:00', '04:00', '08:00', '12:00', '16:00', '20:00', 'Now'],
    datasets: [
      { label: 'Temperature °C', data: [21, 20, 22, 27, 26, 25, 24.8], yAxisID: 'y' },
      { label: 'Humidity %', data: [68, 72, 65, 54, 58, 61, 62], yAxisID: 'y1' },
    ],
  },
  'Last 7 days': {
    labels: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun'],
    datasets: [
      { label: 'Temperature °C', data: [21, 20, 22, 27, 26, 25, 24.8], yAxisID: 'y' },
      { label: 'Humidity %', data: [68, 72, 65, 54, 58, 61, 62], yAxisID: 'y1' },
    ],
  },
  'Last 30 days': {
    labels: ['Week 1', 'Week 2', 'Week 3', 'Week 4', 'Today'],
    datasets: [
      { label: 'Temperature °C', data: [21, 22, 24, 26, 24.8], yAxisID: 'y' },
      { label: 'Humidity %', data: [68, 65, 58, 61, 62], yAxisID: 'y1' },
    ],
  },
};

export function mountChartWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig(DEFAULTS, config);
  const ranges = Array.isArray(options.ranges) && options.ranges.length
    ? options.ranges.map((range) => typeof range === 'object'
      ? { value: String(range.value), label: String(range.label ?? range.value) }
      : { value: String(range), label: String(range) })
    : DEFAULTS.ranges.map((range) => ({ value: range, label: range }));
  const { card, body } = createWidgetCard(container, {
    kind: options.label,
    title: options.title,
    icon: options.icon,
    color: options.color,
  });
  if (options.colSpan) card.classList.add(...String(options.colSpan).split(' ').filter(Boolean));

  body.innerHTML = `
    <div class="flex flex-wrap items-center justify-end gap-3 mb-4">
      <div class="flex flex-wrap items-center justify-end gap-2" role="group" aria-label="Chart range" data-range-group></div>
    </div>
    <div class="h-64"><canvas data-chart aria-label="${escapeAttribute(options.title)}"></canvas></div>`;
  const rangeGroup = body.querySelector('[data-range-group]');
  const rangeButtons = ranges.map((range) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.dataset.range = range.value;
    button.textContent = range.label;
    button.className = 'rounded-xl border px-2.5 py-1.5 text-[11px] font-medium transition-colors border-border bg-background text-foreground';
    rangeGroup.appendChild(button);
    return button;
  });

  let selectedRange = ranges.find((range) => DEMO_DATA[range.value])?.value || ranges[0].value;

  function setActiveRange(nextRange) {
    selectedRange = nextRange;
    rangeButtons.forEach((button) => {
      const isSelected = button.dataset.range === nextRange;
      button.classList.toggle('border-primary', isSelected);
      button.classList.toggle('bg-primary/10', isSelected);
      button.classList.toggle('text-primary', isSelected);
      button.classList.toggle('border-border', !isSelected);
      button.classList.toggle('bg-background', !isSelected);
      button.classList.toggle('text-foreground', !isSelected);
    });
  }

  setActiveRange(selectedRange);

  const canvas = body.querySelector('[data-chart]');
  const chart = new Chart(canvas, {
    type: options.chartType,
    data: options.apiUrl
      ? { labels: [], datasets: [] }
      : formatChartData(DEMO_DATA[selectedRange] || makeFallbackData(selectedRange)),
    options: {
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: { position: 'bottom', labels: { color: '#a1a8ba', usePointStyle: true, boxWidth: 7, padding: 18 } },
      },
      scales: {
        x: { grid: { display: false }, ticks: { color: '#737b90', maxTicksLimit: 7 } },
        y: { position: 'left', grid: { color: 'rgba(148,163,184,.1)' }, ticks: { color: '#737b90' } },
        y1: { position: 'right', grid: { display: false }, ticks: { color: '#737b90' } },
      },
    },
  });

  let requestInProgress = false;
  async function refresh() {
    if (!options.apiUrl || requestInProgress) return;
    requestInProgress = true;
    try {
      const requestUrl = new URL(options.apiUrl, window.location.href);
      if (options.rangeParam) requestUrl.searchParams.set(options.rangeParam, selectedRange);
      const response = await fetch(requestUrl, { headers: { Accept: 'application/json' } });
      if (!response.ok) throw new Error(`Chart request failed (${response.status})`);
      const payload = await response.json();
      const chartData = {
        labels: getValueAtPath(payload, options.labelsPath),
        datasets: getValueAtPath(payload, options.datasetsPath),
      };
      if (!Array.isArray(chartData.labels) || !Array.isArray(chartData.datasets)) {
        throw new Error('Chart API must return labels and datasets arrays at the configured paths.');
      }
      chart.data.labels = chartData.labels;
      chart.data.datasets = formatDatasets(chartData.datasets);
      chart.update();
    } catch (error) {
      console.error(`Chart widget (${options.title}):`, error);
    } finally {
      requestInProgress = false;
    }
  }

  rangeButtons.forEach((button) => {
    button.addEventListener('click', () => {
      setActiveRange(button.dataset.range);
      if (options.apiUrl) refresh();
      else {
        chart.data = formatChartData(DEMO_DATA[selectedRange] || makeFallbackData(selectedRange));
        chart.update();
      }
    });
  });
  refresh();
  const pollIntervalMs = Number(options.pollIntervalMs);
  if (options.apiUrl && pollIntervalMs >= 500) window.setInterval(refresh, pollIntervalMs);
}

function formatChartData(data) {
  return { labels: data.labels, datasets: formatDatasets(data.datasets) };
}

function formatDatasets(datasets) {
  return datasets.map((dataset, index) => ({
    ...dataset,
    borderColor: dataset.borderColor || SERIES_COLORS[index % SERIES_COLORS.length],
    backgroundColor: dataset.backgroundColor || (index === 0 ? 'rgba(56,213,209,.12)' : 'transparent'),
    fill: dataset.fill ?? index === 0,
    tension: dataset.tension ?? 0.4,
    pointRadius: dataset.pointRadius ?? 2,
    borderWidth: dataset.borderWidth ?? 2,
  }));
}

function makeFallbackData(range) {
  return { labels: [range], datasets: [{ label: 'Value', data: [0] }] };
}

function escapeAttribute(value) {
  return String(value ?? '').replace(/[&"<>]/g, (character) => ({
    '&': '&amp;', '"': '&quot;', '<': '&lt;', '>': '&gt;',
  })[character]);
}
