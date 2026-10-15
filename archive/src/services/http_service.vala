/* http_service.vala - libsoup3 HTTP client with UA spoofing */
public class HttpService {
    private const string UA = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 " +
        "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36";

    public static async Bytes fetch_bytes_async (string url) throws GLib.Error {
        var session = new Soup.Session ();
        session.user_agent = UA;
        var msg = new Soup.Message ("GET", url);
        var stream = yield session.send_async (msg, GLib.Priority.DEFAULT, null);
        return yield stream.read_bytes_async (int.MAX, GLib.Priority.DEFAULT, null);
    }
}