// Hoja de ruta: indicadores, recorrido de hitos, "ahora mismo" y lista de hitos con sus tareas.
(function () {
  const CW = window.CW;
  if (!CW) return;

  const STATUS_LABEL = { done: "Hecho", doing: "En curso", todo: "Pendiente", blocked: "Bloqueado" };
  const $ = (id) => document.getElementById(id);

  const allTasks = [];
  CW.phases.forEach((phase) => {
    phase.tasks.forEach((task) => {
      task.phase = phase.id;
      task.phaseName = phase.name;
      allTasks.push(task);
    });
  });

  const counts = { done: 0, doing: 0, todo: 0 };
  allTasks.forEach((t) => { counts[t.status] = (counts[t.status] || 0) + 1; });
  const total = allTasks.length;
  const pct = total ? Math.round((counts.done / total) * 100) : 0;
  const openBugs = CW.bugs.filter((b) => String(b.status || "").toLowerCase().indexOf("resuelt") !== 0).length;

  // Indicadores
  document.title = "Hoja de ruta · " + CW.project;
  $("globalRing").style.setProperty("--p", pct);
  $("globalPct").textContent = pct + "%";
  $("globalCount").textContent = counts.done + " / " + total;
  $("statDone").textContent = counts.done;
  $("statDoing").textContent = counts.doing;
  $("statTodo").textContent = counts.todo;
  $("statBugs").textContent = openBugs;
  $("footCounts").textContent = total + " tareas · " + CW.phases.length + " hitos · " + CW.bugs.length + " bugs registrados";

  // Estado calculado de cada hito
  const phaseInfo = CW.phases.map((phase) => {
    const n = phase.tasks.length;
    const done = phase.tasks.filter((t) => t.status === "done").length;
    const doing = phase.tasks.filter((t) => t.status === "doing").length;
    const status = n && done === n ? "done" : (done + doing > 0 ? "doing" : "todo");
    return { phase, n, done, doing, status, pct: n ? Math.round((done / n) * 100) : 0 };
  });
  const phasesDone = phaseInfo.filter((p) => p.status === "done").length;
  $("phaseSummary").textContent = phasesDone + " de " + phaseInfo.length + " completados";

  // Recorrido de hitos
  $("stepper").innerHTML = phaseInfo.map((p) =>
    '<li class="step ' + p.status + '">' +
      '<a href="#phase-' + esc(p.phase.id) + '" data-phase="' + esc(p.phase.id) + '">' +
        '<span class="step-node">' + (p.status === "done" ? "✓" : "") + "</span>" +
        '<span class="step-text"><b>' + esc(shortCode(p.phase.code)) + "</b><span>" + esc(p.phase.name) + "</span></span>" +
        '<span class="step-pct">' + p.pct + "%</span>" +
      "</a>" +
    "</li>"
  ).join("");

  // Ahora mismo: lo ultimo primero
  $("nowList").innerHTML = (CW.now || []).slice().reverse().slice(0, 7).map((item) =>
    '<li class="' + esc(item.status) + '">' +
      '<span class="badge ' + esc(item.status) + '">' + esc(STATUS_LABEL[item.status] || item.status) + "</span>" +
      "<span>" + esc(item.title) + "</span>" +
    "</li>"
  ).join("");

  // Hitos
  const openState = {};
  const container = $("phases");
  phaseInfo.forEach((p) => {
    const phase = p.phase;
    openState[phase.id] = phase.id === CW.currentPhase || p.status === "doing";

    const section = document.createElement("section");
    section.className = "phase " + p.status + (openState[phase.id] ? " open" : "");
    section.id = "phase-" + phase.id;
    section.dataset.phase = phase.id;

    section.innerHTML =
      '<button class="phase-head" type="button" aria-expanded="' + openState[phase.id] + '">' +
        '<span class="phase-code">' + esc(shortCode(phase.code)) + "</span>" +
        '<span class="phase-main">' +
          '<span class="phase-title">' + esc(phase.name) + "</span>" +
          '<span class="phase-sub">' + esc(phase.window) + " · " + p.n + " tareas</span>" +
        "</span>" +
        '<span class="badge ' + p.status + '">' + STATUS_LABEL[p.status] + "</span>" +
        '<span class="phase-progress"><span class="meter"><i style="width:' + p.pct + '%"></i></span>' +
          '<span class="count">' + p.done + "/" + p.n + "</span></span>" +
        '<svg class="caret" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" aria-hidden="true"><path d="m9 6 6 6-6 6"/></svg>' +
      "</button>" +
      '<div class="phase-body">' +
        '<dl class="phase-goal">' +
          "<div><dt>Objetivo</dt><dd>" + esc(phase.goal) + "</dd></div>" +
          "<div><dt>Puerta de salida</dt><dd>" + esc(phase.gate) + "</dd></div>" +
        "</dl>" +
        '<div class="tasks"></div>' +
      "</div>";

    const tasksBox = section.querySelector(".tasks");
    phase.tasks.forEach((task) => tasksBox.appendChild(renderTask(task)));

    const head = section.querySelector(".phase-head");
    head.addEventListener("click", () => {
      section.classList.toggle("open");
      openState[phase.id] = section.classList.contains("open");
      head.setAttribute("aria-expanded", openState[phase.id]);
    });

    container.appendChild(section);
  });

  // Al pulsar un hito del recorrido, abrirlo y desplazarse.
  $("stepper").addEventListener("click", (e) => {
    const a = e.target.closest("a[data-phase]");
    if (!a) return;
    const sec = document.getElementById("phase-" + a.dataset.phase);
    if (!sec) return;
    e.preventDefault();
    sec.classList.add("open");
    openState[a.dataset.phase] = true;
    sec.scrollIntoView({ behavior: "smooth", block: "start" });
  });

  function renderTask(task) {
    const row = document.createElement("div");
    row.className = "task " + task.status;
    row.dataset.search = (task.id + " " + task.title + " " + (task.note || "") + " " + (task.evidence || "") + " " + task.phaseName).toLowerCase();

    let note = "";
    if (task.evidence) note += '<p class="task-note"><b>Evidencia</b> ' + esc(task.evidence) + "</p>";
    if (task.note) note += '<p class="task-note">' + esc(task.note) + "</p>";

    row.innerHTML =
      '<span class="tick" aria-hidden="true">' + (task.status === "done" ? "✓" : "") + "</span>" +
      '<div class="task-main"><p class="task-title">' + esc(task.title) + "</p>" + note + "</div>" +
      '<span class="badge ' + task.status + '">' + (STATUS_LABEL[task.status] || task.status) + "</span>" +
      '<span class="task-id">' + esc(task.id) + "</span>";
    return row;
  }

  // Filtros
  let activeFilter = "all";
  const phaseEls = Array.from(document.querySelectorAll(".phase"));

  function applyFilters() {
    const q = $("search").value.trim().toLowerCase();
    let visible = 0;

    phaseEls.forEach((phaseEl) => {
      let phaseVisible = 0;
      phaseEl.querySelectorAll(".task").forEach((row) => {
        const show = (!q || row.dataset.search.includes(q)) &&
          (activeFilter === "all" || row.classList.contains(activeFilter));
        row.style.display = show ? "" : "none";
        if (show) phaseVisible++;
      });

      phaseEl.style.display = phaseVisible ? "" : "none";
      const open = (q || activeFilter !== "all") ? phaseVisible > 0 : openState[phaseEl.dataset.phase];
      phaseEl.classList.toggle("open", !!open);
      phaseEl.querySelector(".phase-head").setAttribute("aria-expanded", !!open);
      visible += phaseVisible;
    });

    $("metaNote").textContent = visible + " de " + total + " tareas";
  }

  $("chips").addEventListener("click", (event) => {
    const btn = event.target.closest(".chip");
    if (!btn) return;
    $("chips").querySelectorAll(".chip").forEach((c) => c.classList.remove("active"));
    btn.classList.add("active");
    activeFilter = btn.dataset.filter;
    applyFilters();
  });

  let debounce;
  $("search").addEventListener("input", () => {
    clearTimeout(debounce);
    debounce = setTimeout(applyFilters, 120);
  });
  $("search").addEventListener("keydown", (e) => {
    if (e.key === "Escape") { $("search").value = ""; applyFilters(); }
  });

  applyFilters();

  function shortCode(code) {
    return String(code || "").replace(/^Hito\s+/i, "");
  }

  function esc(str) {
    return String(str == null ? "" : str)
      .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
  }
})();
