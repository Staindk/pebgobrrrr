module.exports = [
  {
    "type": "heading",
    "defaultValue": "PebGoBrrrr"
  },
  {
    "type": "text",
    "defaultValue": "Adjust vibration timings until the pattern is comfortable to read on your wrist. Changes take effect next time the app fires."
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Pattern"
      },
      {
        "type": "select",
        "messageKey": "PatternMode",
        "label": "Vibration pattern",
        "defaultValue": "0",
        "options": [
          { "label": "Digit Groups", "value": "0" },
          { "label": "Morse",        "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "OutputMode",
        "label": "Time detail",
        "defaultValue": "0",
        "options": [
          { "label": "Full time (HH:MM)",        "value": "0" },
          { "label": "Hours only",               "value": "1" },
          { "label": "Round mins to nearest 10", "value": "2" }
        ]
      },
      {
        "type": "toggle",
        "messageKey": "LongBuzzHours",
        "label": "Long buzz for hours",
        "defaultValue": false,
        "description": "Use long buzzes for the hour count, short buzzes for minutes.",
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Buzz lengths"
      },
      {
        "type": "text",
        "defaultValue": "Short buzz is one digit tap or Morse dot. Long buzz is a Morse dash."
      },
      {
        "type": "slider",
        "messageKey": "BuzzShortMs",
        "label": "Short buzz (ms)",
        "defaultValue": 120,
        "min": 50,
        "max": 200,
        "step": 10
      },
      {
        "type": "slider",
        "messageKey": "BuzzLongMs",
        "label": "Long buzz (ms)",
        "defaultValue": 360,
        "min": 100,
        "max": 600,
        "step": 10
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Gap lengths"
      },
      {
        "type": "text",
        "defaultValue": "Digit gap separates individual buzzes or Morse symbols. Char gap separates digits within a group. Group gap separates hours from minutes - make this long enough to feel like a clear break."
      },
      {
        "type": "slider",
        "messageKey": "GapDigitMs",
        "label": "Digit gap (ms)",
        "defaultValue": 120,
        "min": 50,
        "max": 200,
        "step": 10
      },
      {
        "type": "slider",
        "messageKey": "GapCharMs",
        "label": "Char gap (ms)",
        "defaultValue": 350,
        "min": 150,
        "max": 600,
        "step": 10
      },
      {
        "type": "slider",
        "messageKey": "GapGroupMs",
        "label": "Group gap (ms)",
        "defaultValue": 800,
        "min": 400,
        "max": 1500,
        "step": 50
      }
    ]
  },

  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
