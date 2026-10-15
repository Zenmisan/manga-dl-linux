public class MangaDlApplication : Gtk.Application {
    public MangaDlApplication () {
        set_application_id ("org.mangadl.linux");
    }

    protected override void activate () {
        print ("Application activated\n");
        var window = new Gtk.ApplicationWindow (this);
        window.title = "manga-dl";
        window.set_default_size (1200, 800);
        window.show ();
    }
}