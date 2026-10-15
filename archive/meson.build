project('manga-dl', 'vala',
  version: '1.0.0',
  default_options: ['warning_level=2'])

vala = meson.get_compiler('vala')

deps = [
  dependency('gtk4'),
  dependency('sqlite3'),
  dependency('libsoup-3.0'),
  dependency('glib-2.0'),
  dependency('gio-2.0'),
  vala.find_library('posix'),
]

executable('manga-dl',
  sources: [
    'src/main.vala',
    'src/app.vala',
    'src/db/database.vala',
    'src/services/http_service.vala',
    'src/services/extension_manager.vala',
    'src/views/library_view.vala',
    'src/views/search_view.vala',
    'src/views/browse_view.vala',
    'src/views/detail_view.vala',
    'src/views/reader_view.vala',
  ],
  vala_args: [],
  dependencies: deps,
  install: true,
)