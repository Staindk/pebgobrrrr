var Clay = require('@rebble/clay');
var clayConfig = require('./config');

function customClay() {
  var clay = this;

  function updateConditionalItems() {
    var patternMode = clay.getItemByMessageKey('PatternMode').get();
    var longHoursBuzz = clay.getItemByMessageKey('LongBuzzHours');

    if (patternMode === '0') {
      longHoursBuzz.show();
    } else {
      longHoursBuzz.hide();
    }
  }

  clay.on(clay.EVENTS.AFTER_BUILD, function() {
    var patternMode = clay.getItemByMessageKey('PatternMode');
    updateConditionalItems();
    patternMode.on('change', updateConditionalItems);
  });
}

new Clay(clayConfig, customClay);

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready for PebGoBrrrr');
});
