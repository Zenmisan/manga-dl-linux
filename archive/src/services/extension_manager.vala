public class ExtensionManager {
    private string extensions_dir;

    public ExtensionManager () {
        extensions_dir = Path.build_filename (
            Environment.get_user_data_dir (), "manga-dl", "extensions");
        DirUtils.create_with_parents (extensions_dir, 0755);
    }

    public string load_extension (string source_id) {
        var js_path = Path.build_filename (
            extensions_dir, source_id + ".js");
        
        string js_source = "";
        FileUtils.get_contents (js_path, out js_source);
        return js_source;
    }
}