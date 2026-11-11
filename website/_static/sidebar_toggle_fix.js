// Workaround for sphinx-book-theme 1.4.0 with pydata-sphinx-theme 0.20.0:
// both themes attach their sidebar handlers with
// document.querySelector(".primary-toggle"), i.e. to the FIRST such button,
// which is the top-bar icon this layout hides. The visible button above the
// article then does nothing. Forward its clicks to the first button so the
// themes' own handlers run (collapse on desktop, drawer on small screens).
(function () {
  "use strict";
  document.addEventListener("DOMContentLoaded", function () {
    [".primary-toggle", ".secondary-toggle"].forEach(function (selector) {
      var buttons = document.querySelectorAll(selector);
      var first = buttons[0];
      for (var i = 1; i < buttons.length; i++) {
        buttons[i].addEventListener("click", function (ev) {
          ev.preventDefault();
          ev.stopPropagation();
          first.click();
        });
      }
    });
  });
})();
