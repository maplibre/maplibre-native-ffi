/**
 * An HTTP origin the conformance cases point the library at.
 *
 * Node, Bun, and Deno all implement `node:http`, so the three runners that can
 * listen share one origin rather than writing the same server three times. A
 * host that cannot listen leaves the `httpOrigin` capability out and never
 * registers the cases that need one.
 *
 * The origin answers every path with the same empty style, because what these
 * cases read is the request rather than the response.
 */

import {
  EMPTY_STYLE,
  type HttpOrigin,
  type RecordedRequest,
} from "../src/conformance/harness.ts";
import { createServer } from "node:http";
import type { Socket } from "node:net";

export async function startHttpOrigin(): Promise<HttpOrigin> {
  const requests: RecordedRequest[] = [];
  const sockets = new Set<Socket>();

  const server = createServer((request, response) => {
    const headers = new Map<string, string>();
    for (const [name, value] of Object.entries(request.headers)) {
      if (value !== undefined) {
        headers.set(name, Array.isArray(value) ? value.join(", ") : value);
      }
    }
    requests.push({ path: request.url ?? "", headers });
    response.writeHead(200, { "content-type": "application/json" });
    response.end(EMPTY_STYLE);
  });
  server.on("connection", (socket: Socket) => {
    sockets.add(socket);
    socket.on("close", () => sockets.delete(socket));
  });

  // Port zero asks the host for a free one, so two runs, or two suites, never
  // collide on a number this file picked.
  await new Promise<void>((resolve) => {
    server.listen(0, "127.0.0.1", resolve);
  });
  const address = server.address();
  if (address === null || typeof address === "string") {
    throw new Error("the origin is not listening on a port");
  }

  return {
    url: `http://127.0.0.1:${address.port}/`,
    requests,
    close() {
      // MapLibre's HTTP client keeps its connection alive, so closing the
      // listener alone leaves a socket nobody will use again holding the
      // host's event loop open past the end of the run.
      for (const socket of sockets) {
        socket.destroy();
      }
      server.close();
    },
  };
}
