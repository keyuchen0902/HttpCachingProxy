FROM gcc:9

RUN mkdir /var/log/erss
RUN touch /var/log/erss/proxy.log
add . /var/log/erss/
WORKDIR /var/log/erss

ENTRYPOINT ["./run.sh"]