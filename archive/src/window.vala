/* window.vala - Main application window */
public class MangaDlWindow : Adw.ApplicationWindow {
    public MangaDlWindow () {
        title = "manga-dl";
        default_width = 1200;
        default_height = 800;

        /* Dark theme */
        Adw.StyleManager.get_default ().color_scheme = Adw.ColorScheme.FORCE_DARK;

        /* Create navigation view */
        var nav = new Adw.NavigationView ();
        content = nav;
    }
}