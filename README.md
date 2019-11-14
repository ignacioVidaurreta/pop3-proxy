# Trabajo Práctico de Protocolos de Comunicación - POP3

# How to run the project
To run our project make sure you have installed the **make** utility. If you don't,
you can install it with the apt-get utility running `sudo apt-get install make`

## To compile & run the proxy, just run:
`make run` which will start it with localhost as default IP. To run it manually:
`make && ./pop3filter [IP_ADDR]`

## To run the admin protocol pop3ctl:
In terminal A: Compile and run the pop3filter
In terminal B: Move to the admin folder (`cd admin`) and run `make run`
You can also break the `make run` into two steps which are `make && ./pop3ctl [args]`

## To run the filter standalone
Move to the mr-mime folder (`cd mr-mime`), compile it (`make`) and run it with any mail you want to apply the filter:
`./stripmime [MAIL_FILE]`


# Authors
- Ignacio Matías Vidaurreta
- Clara Guzzetti
- José Martín Torreguitar
- Juan Bensadon
