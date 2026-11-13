/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  darkMode: 'class',
  theme: {
    extend: {
      colors: {
        board: {
          light: '#f0d9b5',
          dark: '#b58863',
          lightHighlight: '#cdd26a',
          darkHighlight: '#aaa23a',
          selectLight: '#bae1ff',
          selectDark: '#7fc4fd',
        },
        surface: {
          dark: '#1e222b',
          darker: '#161920',
          panel: '#262b36',
          border: '#373e4d',
        }
      },
    },
  },
  plugins: [],
}

