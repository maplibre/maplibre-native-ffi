// The HTTP origin of the ArkTS suite, which runs on the build machine and
// which the emulator reaches through `hdc rport`. A case on the device opens
// an origin with fixed bodies by path, reads the requests it recorded, and
// closes it, all through this server:
//
//   POST   /origins                 {"routes": {path: body}} -> {"id": n}
//   GET    /origins/<id>/requests   -> [{"path", "headers"}]
//   DELETE /origins/<id>
//   ANY    /o/<id><path>            the origin itself
//
//   node test/arkts/origin.ts <port-file>
//
// The server listens on a free port on the loopback interface, writes the
// port to <port-file>, and runs until it is killed.

import { writeFileSync } from "node:fs";
import {
  createServer,
  type IncomingHttpHeaders,
  type IncomingMessage,
} from "node:http";
import type { AddressInfo } from "node:net";
import process from "node:process";

interface Origin {
  readonly routes: Readonly<Record<string, string>>;
  readonly requests: { path: string; headers: IncomingHttpHeaders }[];
}

const origins = new Map<number, Origin>();
let next = 1;

function body(request: IncomingMessage): Promise<string> {
  return new Promise((resolve, reject) => {
    let text = "";
    request.setEncoding("utf8");
    request.on("data", (chunk: string) => (text += chunk));
    request.on("end", () => resolve(text));
    request.on("error", reject);
  });
}

const server = createServer((request, response) => {
  const url = request.url ?? "/";
  const json = (status: number, value: unknown) =>
    response
      .writeHead(status, { "content-type": "application/json" })
      .end(JSON.stringify(value));
  const served = /^\/o\/(\d+)(\/.*)$/.exec(url);
  if (served) {
    const origin = origins.get(Number(served[1]));
    const path = served[2]!;
    origin?.requests.push({ path, headers: request.headers });
    const routed = origin?.routes[path];
    if (routed === undefined) {
      response.writeHead(404).end();
    } else {
      response
        .writeHead(200, { "content-type": "application/json" })
        .end(routed);
    }
    return;
  }
  if (url === "/origins" && request.method === "POST") {
    void body(request).then((text) => {
      const id = next++;
      const { routes } = JSON.parse(text) as { routes: Record<string, string> };
      origins.set(id, { routes, requests: [] });
      json(200, { id });
    });
    return;
  }
  const control = /^\/origins\/(\d+)(\/requests)?$/.exec(url);
  const origin = control ? origins.get(Number(control[1])) : undefined;
  if (control && origin && control[2] && request.method === "GET") {
    json(200, origin.requests);
  } else if (control && origin && request.method === "DELETE") {
    origins.delete(Number(control[1]));
    response.writeHead(204).end();
  } else {
    response.writeHead(404).end();
  }
});

server.listen(0, "127.0.0.1", () => {
  writeFileSync(
    process.argv[2]!,
    String((server.address() as AddressInfo).port),
  );
});
