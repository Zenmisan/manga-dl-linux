/* reader_view.vala - Manga reader view */
public class ReaderView : Gtk.ScrolledWindow {
    public ReaderView () {
        set_policy (Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC);
        set_margin_top (12);
        set_margin_bottom (12);
    }
}