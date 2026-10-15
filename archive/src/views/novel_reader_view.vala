/* novel_reader_view.vala - Novel reader view */
public class NovelReaderView : Gtk.ScrolledWindow {
    public NovelReaderView () {
        set_policy (Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        set_margin_top (12);
        set_margin_bottom (12);
    }
}