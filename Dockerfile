FROM alpine:3.9 AS builder
RUN apk --no-cache add build-base
COPY . ./BuggyApp
WORKDIR /BuggyApp/
RUN make clean && make

FROM alpine:3.9
RUN apk --no-cache add libgcc libstdc++
COPY --from=builder ./BuggyApp/BuggyApp ./buggyapp
CMD ./buggyapp
EXPOSE 8080
