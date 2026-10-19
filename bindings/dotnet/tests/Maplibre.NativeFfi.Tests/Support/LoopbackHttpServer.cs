using System.Net;
using System.Net.Sockets;
using System.Text;

namespace Maplibre.NativeFfi.Tests;

/// <summary>A loopback HTTP server that serves fixed bodies and records each requested path.</summary>
/// <remarks>
/// It answers one request per connection and closes it, and answers a path it does not serve
/// with 404.
/// </remarks>
internal sealed class LoopbackHttpServer : IDisposable
{
    private readonly TcpListener listener = new(IPAddress.Loopback, 0);
    private readonly IReadOnlyDictionary<string, byte[]> bodies;
    private readonly List<string> requested = [];

    internal LoopbackHttpServer(IReadOnlyDictionary<string, byte[]> bodies)
    {
        this.bodies = bodies;
        listener.Start();
        _ = Task.Run(AcceptAsync);
    }

    /// <summary>The paths requested so far, in arrival order.</summary>
    internal IReadOnlyList<string> RequestedPaths
    {
        get
        {
            lock (requested)
                return [.. requested];
        }
    }

    internal string Url(string path) =>
        $"http://127.0.0.1:{((IPEndPoint)listener.LocalEndpoint).Port}{path}";

    public void Dispose() => listener.Stop();

    private async Task AcceptAsync()
    {
        while (true)
        {
            TcpClient client;
            try
            {
                client = await listener.AcceptTcpClientAsync();
            }
            catch (Exception exception)
                when (exception is SocketException or ObjectDisposedException)
            {
                return;
            }
            _ = Task.Run(() => RespondAsync(client));
        }
    }

    private async Task RespondAsync(TcpClient client)
    {
        using (client)
        {
            try
            {
                var stream = client.GetStream();
                using var reader = new StreamReader(stream, Encoding.ASCII, leaveOpen: true);
                var requestLine = await reader.ReadLineAsync() ?? "";
                while (!string.IsNullOrEmpty(await reader.ReadLineAsync())) { }
                var parts = requestLine.Split(' ');
                var path = parts.Length > 1 ? parts[1] : "";
                lock (requested)
                    requested.Add(path);
                var found = bodies.TryGetValue(path, out var body);
                body ??= [];
                var head = Encoding.ASCII.GetBytes(
                    $"HTTP/1.1 {(found ? "200 OK" : "404 Not Found")}\r\n"
                        + $"Content-Length: {body.Length}\r\nConnection: close\r\n\r\n"
                );
                await stream.WriteAsync(head);
                await stream.WriteAsync(body);
            }
            catch (IOException)
            {
                // The client went away; its request is already recorded.
            }
        }
    }
}
