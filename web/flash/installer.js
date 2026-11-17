"use strict";

const select = document.getElementById("firmware-version");
const status = document.getElementById("release-status");
const installer = document.getElementById("installer");
const installButton = document.getElementById("install-button");
const retry = document.getElementById("retry-button");
let releases = [];
let toolsReady = false;

// Build the note content with DOM nodes so release Markdown never becomes HTML.
function appendNoteText(element, text) {
  const tokens = /(\*\*[^*]+\*\*|`[^`]+`|\[[^\]]+\]\(https?:\/\/[^\s)]+\)|https?:\/\/[^\s<>]+)/g;
  let previous = 0;
  for (const match of text.matchAll(tokens)) {
    element.append(document.createTextNode(text.slice(previous, match.index)));
    const token = match[0];
    let node;
    if (token.startsWith("**")) {
      node = document.createElement("strong");
      node.textContent = token.slice(2, -2);
    } else if (token.startsWith("`")) {
      node = document.createElement("code");
      node.textContent = token.slice(1, -1);
    } else {
      const link = /^\[([^\]]+)\]\((https?:\/\/[^\s)]+)\)$/.exec(token);
      node = document.createElement("a");
      node.textContent = link ? link[1] : token;
      node.href = link ? link[2] : token;
    }
    element.append(node);
    previous = match.index + token.length;
  }
  element.append(document.createTextNode(text.slice(previous)));
}

function renderReleaseNotes(notes) {
  const body = document.getElementById("release-notes-body");
  body.replaceChildren();
  let paragraph = null;
  let list = null;
  for (const line of (notes.trim() || "No release notes were provided for this version.").split(/\r?\n/)) {
    const heading = /^#{1,6}\s+(.+)$/.exec(line);
    const item = /^\s*(?:[-*+] |\d+\. )(.+)$/.exec(line);
    if (!line.trim()) {
      paragraph = list = null;
    } else if (heading) {
      const node = document.createElement("h3");
      appendNoteText(node, heading[1]);
      body.append(node);
      paragraph = list = null;
    } else if (item) {
      const tag = /^\s*\d+\./.test(line) ? "ol" : "ul";
      if (!list || list.localName !== tag) {
        list = document.createElement(tag);
        body.append(list);
      }
      const node = document.createElement("li");
      appendNoteText(node, item[1]);
      list.append(node);
      paragraph = null;
    } else {
      if (!paragraph) {
        paragraph = document.createElement("p");
        body.append(paragraph);
      } else {
        paragraph.append(document.createTextNode(" "));
      }
      appendNoteText(paragraph, line);
      list = null;
    }
  }
}

function setLink(id, href) {
  const link = document.getElementById(id);
  link.hidden = !href;
  if (href) link.href = href;
  else link.removeAttribute("href");
}

function updateSelection() {
  const release = releases[Number(select.value)];
  installButton.disabled = !release || !toolsReady;
  installer.hidden = !release;
  document.getElementById("release-notes").hidden = !release;
  if (!release) {
    installer.removeAttribute("manifest");
    document.getElementById("release-notes-body").replaceChildren();
    document.getElementById("beta-note").hidden = true;
    ["notes-link", "notes-source", "factory-link", "ota-link"].forEach(id => setLink(id, null));
    return;
  }
  installer.setAttribute("manifest", release.manifest);
  document.getElementById("beta-note").hidden = !release.prerelease;
  setLink("notes-link", "#release-notes");
  setLink("notes-source", release.url);
  document.getElementById("release-notes-title").textContent = `Release notes · ${release.tag}`;
  renderReleaseNotes(release.notes || "");
  setLink("factory-link", release.factory);
  setLink("ota-link", release.ota);
  const date = release.published ? new Date(release.published).toLocaleDateString() : "";
  status.textContent = `${release.tag}${date ? " · Published " + date : ""} · ESP32`;
}

async function loadReleases() {
  select.disabled = true;
  retry.hidden = true;
  releases = [];
  updateSelection();
  status.textContent = "Loading GitHub Releases…";
  try {
    const response = await fetch("release-index.json", { cache: "no-store" });
    if (!response.ok) throw new Error(`Release list unavailable (HTTP ${response.status}).`);
    const index = await response.json();
    if (!Array.isArray(index.releases)) throw new Error("The release list is invalid.");
    releases = index.releases.filter(release => release.manifest && release.factory);
    setLink("releases-link", `https://github.com/${index.repository}/releases`);
    select.replaceChildren();
    if (!releases.length) {
      select.add(new Option("No installable releases yet", ""));
      status.textContent = "No full-install images have been published yet. Check GitHub Releases for updates.";
      return;
    }
    releases.forEach((release, i) => {
      select.add(new Option(`${release.name}${release.prerelease ? " (prerelease)" : i === 0 ? " (latest stable)" : ""}`, String(i)));
    });
    select.value = "0";
    select.disabled = false;
    updateSelection();
  } catch (error) {
    select.replaceChildren(new Option("Releases unavailable", ""));
    status.textContent = `${error.message} Try again in a moment.`;
    retry.hidden = false;
  }
}

select.addEventListener("change", updateSelection);
retry.addEventListener("click", loadReleases);
const toolsStatus = document.getElementById("tools-status");
const toolsTimeout = setTimeout(() => {
  toolsStatus.textContent = "USB installer could not load. Check your connection and reload the page. Firmware downloads are still available.";
  toolsStatus.hidden = false;
}, 15000);
customElements.whenDefined("esp-web-install-button").then(() => {
  clearTimeout(toolsTimeout);
  toolsReady = true;
  toolsStatus.hidden = true;
  updateSelection();
});
loadReleases();
