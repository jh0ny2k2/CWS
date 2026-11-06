// Servidor estatico minimo para el panel de seguimiento (sin dependencias).
// Uso:  node web/server.js [puerto]
// Por defecto: http://127.0.0.1:8100

const http = require("http");
const fs = require("fs");
const path = require("path");

const ROOT = __dirname;
const PORT = Number(process.argv[2] || process.env.CW_PANEL_PORT || 8100);
const HOST = "127.0.0.1";

const TYPES = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".svg": "image/svg+xml",
  ".ico": "image/x-icon",
  ".woff2": "font/woff2",
  ".txt": "text/plain; charset=utf-8"
};

const server = http.createServer((req, res) => {
  const urlPath = decodeURIComponent((req.url || "/").split("?")[0]);
  const relative = urlPath === "/" ? "index.html" : urlPath.replace(/^\/+/, "");
  const target = path.resolve(ROOT, relative);

  // Evitar salir de la carpeta web/.
  if (target !== ROOT && !target.startsWith(ROOT + path.sep)) {
    res.writeHead(403, { "Content-Type": "text/plain; charset=utf-8" });
    res.end("403 Prohibido");
    return;
  }

  fs.stat(target, (err, stat) => {
    if (err || !stat.isFile()) {
      res.writeHead(404, { "Content-Type": "text/plain; charset=utf-8" });
      res.end("404 No encontrado: " + relative);
      return;
    }

    const ext = path.extname(target).toLowerCase();
    res.writeHead(200, {
      "Content-Type": TYPES[ext] || "application/octet-stream",
      "Cache-Control": "no-store, no-cache, must-revalidate",
      "Content-Length": stat.size
    });
    fs.createReadStream(target).pipe(res);
  });
});

server.on("error", (err) => {
  if (err.code === "EADDRINUSE") {
    console.error("El puerto " + PORT + " ya esta ocupado. Prueba: node web/server.js 8101");
  } else {
    console.error(err);
  }
  process.exit(1);
});

server.listen(PORT, HOST, () => {
  console.log("Panel Construction World: http://" + HOST + ":" + PORT);
  console.log("Hoja de ruta:            http://" + HOST + ":" + PORT + "/index.html");
  console.log("Kanban:                  http://" + HOST + ":" + PORT + "/kanban.html");
  console.log("Ctrl+C para parar.");
});
