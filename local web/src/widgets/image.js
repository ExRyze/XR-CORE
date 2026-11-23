import { createWidgetCard, mergeWidgetConfig } from './widget-base.js';

const DEFAULTS = {
  title: 'Image',
  label: 'Image',
  icon: 'image',
  color: 'neutral',
  imageUrl: '',
  alt: '',
  caption: '',
  detail: '',
  badge: '',
  colSpan: '',
  ratio: '16 / 9',
};

export function mountImageWidget(container, config = {}) {
  if (!container) return;
  const options = mergeWidgetConfig(DEFAULTS, config);
  const { card, body } = createWidgetCard(container, {
    kind: options.label,
    title: options.title,
    icon: options.icon,
    color: options.color,
  });
  if (options.colSpan) card.classList.add(...String(options.colSpan).split(' ').filter(Boolean));

  const ratio = String(options.ratio || '16 / 9').trim() || '16 / 9';
  body.innerHTML = `
    <div class="relative w-full overflow-hidden rounded-xl bg-muted" data-image-frame style="aspect-ratio: ${ratio};">
      <img class="w-full h-full object-cover" data-image>
      <div class="absolute inset-0 bg-gradient-to-t from-black/60 via-transparent to-transparent pointer-events-none"></div>
      <span class="absolute bottom-3 left-4 right-24 text-xs text-white" data-caption></span>
      <span class="absolute bottom-3 right-4 text-xs text-white/80" data-detail></span>
      <span class="absolute top-3 right-3 text-[10px] uppercase tracking-wider rounded-full px-2.5 py-1 hidden" data-badge></span>
      <span class="absolute inset-0 grid place-items-center text-xs text-muted-foreground hidden" data-error>Image unavailable</span>
    </div>`;

  const image = body.querySelector('[data-image]');
  const caption = body.querySelector('[data-caption]');
  const detail = body.querySelector('[data-detail]');
  const badge = body.querySelector('[data-badge]');
  const error = body.querySelector('[data-error]');
  image.alt = options.alt;
  image.loading = 'lazy';
  if (options.imageUrl) image.src = options.imageUrl;
  else {
    image.classList.add('hidden');
    error.classList.remove('hidden');
  }
  caption.textContent = options.caption;
  detail.textContent = options.detail;
  if (options.badge) {
    badge.textContent = options.badge;
    badge.className += ' text-danger bg-danger/10 border border-danger/20';
    badge.classList.remove('hidden');
  }
  image.addEventListener('error', () => {
    image.classList.add('hidden');
    error.classList.remove('hidden');
  });
}
