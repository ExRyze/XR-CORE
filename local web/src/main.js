import './api.js';
import './style.css';
import { createIcons, Activity, BadgeInfo, Battery, Cpu, Droplets, Fan, Gauge, Image, LayoutDashboard, Lightbulb, Loader, Lock, LogIn, Radar, Radio, RadioTower, RefreshCw, RotateCw, Save, Settings, ShieldAlert, Signal, SlidersHorizontal, Sprout, TabletSmartphone, Tag, Thermometer, Trash2, Unlock, Upload, Waves, Wifi, Wind, Zap } from 'lucide';
import { mountValueWidget } from './widgets/value.js';
import { mountSwitchWidget } from './widgets/switch.js';
import { mountStepperWidget } from './widgets/stepper.js';
import { mountValueDropdownWidget } from './widgets/value-dropdown.js';
import { mountStatusWidget } from './widgets/status.js';
import { mountPercentageWidget } from './widgets/percentage.js';
import { mountChartWidget } from './widgets/chart.js';
import { mountImageWidget } from './widgets/image.js';
import { mountValueInputWidget } from './widgets/value-input.js';
import { mountValueSelectorWidget } from './widgets/value-selector.js';
import { mountSliderWidget } from './widgets/slider.js';
import { mountPushButtonWidget } from './widgets/push-button.js';

const icons = {
  Activity: Activity,
  BadgeInfo: BadgeInfo,
  Battery: Battery,
  Cpu: Cpu,
  Droplets: Droplets,
  Fan: Fan,
  Gauge: Gauge,
  Image: Image,
  LayoutDashboard: LayoutDashboard,
  Lightbulb: Lightbulb,
  Loader: Loader,
  Lock: Lock,
  LogIn: LogIn,
  Radar: Radar,
  Radio: Radio,
  RadioTower: RadioTower,
  RefreshCw: RefreshCw,
  RotateCw: RotateCw,
  Save: Save,
  Settings: Settings,
  ShieldAlert: ShieldAlert,
  Signal: Signal,
  SlidersHorizontal: SlidersHorizontal,
  Sprout: Sprout,
  TabletSmartphone: TabletSmartphone,
  Tag: Tag,
  Thermometer: Thermometer,
  Trash2: Trash2,
  Unlock: Unlock,
  Upload: Upload,
  Waves: Waves,
  Wifi: Wifi,
  Wind: Wind,
  Zap: Zap,
};

window.lucide = { createIcons: () => createIcons({ icons }) };
document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('[data-widget="value"]').forEach((container) => {
    mountValueWidget(container, {
      apiUrl: container.dataset.apiUrl,
      title: container.dataset.title,
      unit: container.dataset.unit,
      pollIntervalMs: container.dataset.pollIntervalMs,
      decimals: container.dataset.decimals,
      valuePath: container.dataset.valuePath,
      color: container.dataset.color,
      icon: container.dataset.icon,
    });
  });

  document.querySelectorAll('[data-widget="value-input"]').forEach((container) => {
    mountValueInputWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      min: container.dataset.min,
      max: container.dataset.max,
      step: container.dataset.step,
      unit: container.dataset.unit,
      decimals: container.dataset.decimals,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="value-selector"]').forEach((container) => {
    let choices = [];
    try {
      choices = JSON.parse(container.dataset.choices || '[]');
    } catch (error) {
      console.error('Value selector widget choices must be valid JSON.', container, error);
    }
    mountValueSelectorWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      choices,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="switch"]').forEach((container) => {
    mountSwitchWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      onLabel: container.dataset.onLabel,
      offLabel: container.dataset.offLabel,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="stepper"]').forEach((container) => {
    mountStepperWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      min: container.dataset.min,
      max: container.dataset.max,
      step: container.dataset.step,
      unit: container.dataset.unit,
      decimals: container.dataset.decimals,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="value-dropdown"]').forEach((container) => {
    let choices = [];
    try {
      choices = JSON.parse(container.dataset.choices || '[]');
    } catch (error) {
      console.error('Value dropdown widget choices must be valid JSON.', container, error);
    }
    mountValueDropdownWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      choices,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="status"]').forEach((container) => {
    mountStatusWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      detail: container.dataset.detail,
      apiUrl: container.dataset.apiUrl,
      valuePath: container.dataset.valuePath,
      detailPath: container.dataset.detailPath,
      pollIntervalMs: container.dataset.pollIntervalMs,
    });
  });

  document.querySelectorAll('[data-widget="percentage"]').forEach((container) => {
    mountPercentageWidget(container, {
      apiUrl: container.dataset.apiUrl,
      title: container.dataset.title,
      label: container.dataset.label,
      value: container.dataset.value,
      valuePath: container.dataset.valuePath,
      min: container.dataset.min,
      max: container.dataset.max,
      unit: container.dataset.unit,
      variant: container.dataset.variant,
      detail: container.dataset.detail,
      pollIntervalMs: container.dataset.pollIntervalMs,
      decimals: container.dataset.decimals,
      color: container.dataset.color,
      icon: container.dataset.icon,
    });
  });

  document.querySelectorAll('[data-widget="chart"]').forEach((container) => {
    let ranges;
    try {
      if (container.dataset.ranges) ranges = JSON.parse(container.dataset.ranges);
    } catch (error) {
      console.error('Chart widget ranges must be valid JSON.', container, error);
    }
    mountChartWidget(container, {
      apiUrl: container.dataset.apiUrl,
      title: container.dataset.title,
      label: container.dataset.label,
      icon: container.dataset.icon,
      color: container.dataset.color,
      chartType: container.dataset.chartType,
      ranges,
      rangeParam: container.dataset.rangeParam,
      labelsPath: container.dataset.labelsPath,
      datasetsPath: container.dataset.datasetsPath,
      pollIntervalMs: container.dataset.pollIntervalMs,
      colSpan: container.dataset.colSpan,
    });
  });

  document.querySelectorAll('[data-widget="image"]').forEach((container) => {
    mountImageWidget(container, {
      title: container.dataset.title,
      label: container.dataset.label,
      icon: container.dataset.icon,
      color: container.dataset.color,
      imageUrl: container.dataset.imageUrl,
      alt: container.dataset.alt,
      caption: container.dataset.caption,
      detail: container.dataset.detail,
      badge: container.dataset.badge,
      colSpan: container.dataset.colSpan,
      ratio: container.dataset.ratio,
    });
  });

  document.querySelectorAll('[data-widget="slider"]').forEach((container) => {
    mountSliderWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      value: container.dataset.value,
      min: container.dataset.min,
      max: container.dataset.max,
      step: container.dataset.step,
      unit: container.dataset.unit,
      decimals: container.dataset.decimals,
      apiUrl: container.dataset.apiUrl,
    });
  });

  document.querySelectorAll('[data-widget="push-button"]').forEach((container) => {
    mountPushButtonWidget(container, {
      title: container.dataset.title,
      icon: container.dataset.icon,
      color: container.dataset.color,
      label: container.dataset.label,
      value: container.dataset.value,
      apiUrl: container.dataset.apiUrl,
      busyLabel: container.dataset.busyLabel,
    });
  });
});
