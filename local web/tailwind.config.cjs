module.exports = {
  content: ['./index.html', './login.html', './src/**/*.{js,ts}'],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        background: 'hsl(220 30% 7%)',
        foreground: 'hsl(220 20% 93%)',
        card: 'hsl(220 28% 12%)',
        'card-foreground': 'hsl(220 20% 93%)',
        popover: 'hsl(220 28% 14%)',
        'popover-foreground': 'hsl(220 20% 93%)',
        muted: 'hsl(220 24% 16%)',
        'muted-foreground': 'hsl(220 12% 65%)',
        border: 'hsl(220 20% 18%)',
        input: 'hsl(220 20% 18%)',
        primary: 'hsl(185.03 70.2% 50%)',
        'primary-foreground': 'hsl(220 20% 4%)',
        secondary: 'hsl(155.2 70.2% 50%)',
        'secondary-foreground': 'hsl(220 20% 4%)',
        accent: 'hsl(214.86 70.2% 50%)',
        'accent-foreground': 'hsl(220 20% 4%)',
        success: 'hsl(142.1 76.2% 36.3%)',
        'success-foreground': 'hsl(145, 100%, 90%)',
        danger: 'hsl(0 72.2% 50.6%)',
        'danger-foreground': 'hsl(0, 100%, 95%)',
        warning: 'hsl(38 92% 50%)',
        'warning-foreground': 'hsl(42, 100%, 90%)',
        info: 'hsl(210 100% 56%)',
        'info-foreground': 'hsl(210 100% 95%)'
      }
    }
  },
  plugins: []
};
