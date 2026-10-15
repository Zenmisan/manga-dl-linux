# Needs attention

Things that are waiting on a decision, an account setting, or a value only the project owner can provide.
Anything listed here is deliberately **not** faked in the app: the related UI is hidden or disabled until it
is resolved. Remove an item once it is done.

Last updated: 2026-10-09

## Waiting on the owner

- [ ] **Update channel URL.** Where new Linux releases will be posted (e.g. a GitHub Releases URL). Until it is
      supplied, the fake "update ready / Restart & Install / Install updates automatically" UI in
      System settings and `UpdateDialog.qml` is removed, not mocked. (Plan item: R5, Phase 7.)
- [ ] **Google sign-in redirect.** Add `http://localhost:5678/google-callback` to the Supabase project's
      Redirect URLs allowlist (Authentication → URL Configuration). Needed before Google sign-in can work.
      The owner will do this when the backend reaches Phase 6.
- [ ] **Local server / Remote servers (System settings → Servers).** Undecided. The UI shows a
      `http://localhost:4567 · running` badge and an "Add server" button. The Windows app has the same
      static label with no server behind it. Hidden on Linux until a decision is made.
- [ ] **Old Vala/GTK4 attempt** (root-level `src/`, `meson.build`, `build/`, `DESKTOP-LINUX.md` in the
      parent folder). Proposed: move to an `archive/` folder rather than delete. Nothing is touched without
      the owner's OK. (Plan item: Phase 9.)

## Conditional

- [ ] **AniList / MyAnimeList client IDs.** The plan is to reuse the Windows app's client IDs (the OAuth
      redirect is a loopback `http://localhost:5678/...`, which is the same on Linux). If either provider
      rejects the redirect for Linux, the owner needs to register a Linux-specific app and supply the ID.
- [ ] **Supabase URL / anon key.** Reused from the Windows app. Never commit service keys or OAuth client
      secrets; read them from config or the environment.

## Decided (for the record)

- Trackers: AniList and MyAnimeList only (what Windows implements). Kitsu, MangaUpdates, Shikimori and
  Bangumi are removed from the Linux UI.
- No git commits or pushes are made by the assistant; the owner commits.
- Windows is the reference implementation, but not infallible: its System settings XAML still contains
  `[user]` / `[date]` placeholders and a hardcoded "Version 1.0.0 · up to date". Linux does these properly.

## System dependencies to remember for the README (Phase 9)

- [ ] **WebP pages** (comix.to and others) need Qt's image-format plugins at runtime: Arch `qt6-imageformats`, Debian/Ubuntu `qt6-image-formats-plugins`. Without them `QImage` cannot decode WebP and those pages come back empty.

## Optional, later

- [ ] Flatpak manifest.
