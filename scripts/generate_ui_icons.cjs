// SVG is the editable source. PNG renditions let the Direct2D 1.0/WIC renderer
// display the same artwork without adding a runtime SVG engine dependency.
const fs = require('node:fs');
const path = require('node:path');
const sharp = require('sharp');
const folder = path.join(__dirname, '..', 'assets', 'ui');
const shapes = {
 home: '<path d="M3 10.5 12 3l9 7.5M5 9v12h5v-7h4v7h5V9"/>',
 package: '<path d="m3 7 9-4 9 4-9 4-9-4Zm0 0v10l9 4 9-4V7M12 11v10M7.5 5l9 4"/>',
 monitor: '<rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8m-4-4v4"/>',
 plus: '<path d="M12 4v16M4 12h16"/>',
 refresh: '<path d="M20 8a8 8 0 0 0-13-3L3 9m0-6v6h6m-5 7a8 8 0 0 0 13 3l4-4m0 6v-6h-6"/>',
 download: '<path d="M12 3v12m-5-5 5 5 5-5M4 16v5h16v-5"/>',
 settings: '<path d="m9 3-.6 2.2-2 .9-2-.7-2 3.4 1.6 1.6v2.3L2.4 14l2 3.5 2.1-.6 2 .9L9 20h4l.6-2.2 2-.9 2 .6 2-3.5-1.6-1.4v-2.3L19.6 9l-2-3.5-2 .6-2-.9L13 3Z"/><circle cx="11" cy="11.5" r="3"/>',
 search: '<circle cx="10.5" cy="10.5" r="7"/><path d="m16 16 5 5"/>',
 play: '<path d="m8 4 12 8-12 8Z"/>',
 chevron: '<path d="m6 9 6 6 6-6"/>',
 grid: '<rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/>',
 minimize: '<path d="M4 12h16"/>',
 maximize: '<rect x="4" y="4" width="16" height="16" rx="1"/>',
 restore: '<path d="M8 8V4h12v12h-4"/><rect x="4" y="8" width="12" height="12" rx="1"/>',
 close: '<path d="m5 5 14 14M19 5 5 19"/>'
};
(async () => {
  for (const [name, shape] of Object.entries(shapes)) {
    const svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#c4d8f3" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">${shape}</svg>\n`;
    fs.writeFileSync(path.join(folder, name + '.svg'), svg);
    await sharp(Buffer.from(svg)).resize(144, 144).png().toFile(path.join(folder, name + '.png'));
  }
})();
