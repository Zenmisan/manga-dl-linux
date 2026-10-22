# Needs attention

Things that are waiting on a decision, an account setting, or a value only the project owner can provide.
Anything listed here is deliberately **not** faked in the app: the related UI is hidden or disabled until it
is resolved. Remove an item once it is done.

Last updated: 2026-10-22

## Waiting on the owner

- [x] **Update channel URL.** Defined and wired to the official GitHub Releases channel (`https://github.com/zenmisan/manga-dl/releases`) with live GitHub Releases API checks and download links in `UpdateDialog.qml`.
- [x] **Google sign-in flow.** Configured via Firebase Auth (`z-manga-dl`) loopback listener (`http://localhost:5678/google-login` -> `/auth/callback`) with session token forwarding to Supabase PostgREST sync.
- [x] **Local server / Remote servers.** Cleaned up to reflect client-side standalone execution; all scrapers execute natively on-device.
- [x] **Old Vala/GTK4 attempt.** Safely archived under `archive/DESKTOP-LINUX.md`.

## Conditional

- [x] **AniList / MyAnimeList client IDs.** Configured with loopback OAuth PKCE redirect listeners.
- [x] **Supabase URL / anon key & Firebase config.** Connected and authenticated without committing service secrets.

## Decided (for the record)

- Trackers: AniList and MyAnimeList only (matching Windows implementation).
- Authentication: Supabase GoTrue for email/password; Firebase Auth for Google Sign-In; Supabase PostgREST for library & progress sync.
- Unified Backups: `.mangadl` and `.tachibk` supported bidirectionally with full Web schema parity (`bookmarks`, `settings`, `trackerBinds`, `library`, `progress`, `categories`).
- No git commits or pushes are made by the assistant; the owner commits.

## System dependencies to remember for the README
- [x] **WebP pages** (comix.to and others) need Qt's image-format plugins at runtime: Arch `qt6-imageformats`, Debian/Ubuntu `qt6-image-formats-plugins`. Documented in `README.md`.
