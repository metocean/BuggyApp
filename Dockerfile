FROM gcc
COPY BuggyApp ./buggyapp
CMD ./buggyapp
EXPOSE 8080
