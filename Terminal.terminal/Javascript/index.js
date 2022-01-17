(async function () {
  const authRes = await fetch("/.auth/me");
  const me = await authRes.json();

  if (me.clientPrincipal) {
    window.location.href = "/app";
  } else {
    window.location.href = "/login";
  }
})();
