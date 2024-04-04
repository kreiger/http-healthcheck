FROM alpine:latest as builder

RUN apk add --no-cache gcc musl-dev

COPY http-healthcheck.c /http-healthcheck.c

RUN gcc -static -Os -o http-healthcheck /http-healthcheck.c

RUN strip http-healthcheck

FROM scratch

COPY --from=builder /http-healthcheck /http-healthcheck

ENTRYPOINT ["/http-healthcheck"]
