#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

static void aggiungi_prodotto_ordine(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[4];

	char nomeProdotto[46];
	char aggiunta[46];
	char quantita[10];
	char ordine[10];
	int quantita_int;
	int ordine_int;

	printf("\nNome Prodotto: ");
	getInput(46, nomeProdotto, false);
	printf("\nOrdine: ");
	getInput(10, ordine, false);
	printf("\nQuantità: ");
	getInput(10, quantita, false);
	printf("\nAggiunta: ");
	getInput(46, aggiunta, false);

	quantita_int = atoi(quantita);
	ordine_int = atoi(ordine);

	// Prepare stored procedure call
	if (!setup_prepared_stmt(&prepared_stmt, "call aggiungiProdottoAOrdine(?, ?, ?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &ordine_int;
	param[0].buffer_length = sizeof(ordine_int);

	param[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[1].buffer = nomeProdotto;
	param[1].buffer_length = strlen(nomeProdotto);

	if ((!strcmp(aggiunta, "") || !strcmp(aggiunta, "\0")))
	{
		printf("Aggiunta null\n");
		param[2].buffer_type = MYSQL_TYPE_NULL;
		param[2].is_null = true;

		param[3].buffer_type = MYSQL_TYPE_LONG;
		param[3].buffer = &quantita_int;
		param[3].buffer_length = sizeof(quantita_int);
	}
	else
	{
		printf("Aggiunta non null\n");
		printf("Aggiunta catturata: %s\n", aggiunta);
		param[2].buffer_type = MYSQL_TYPE_VAR_STRING;
		param[2].buffer = aggiunta;
		param[2].buffer_length = strlen(aggiunta);

		param[3].buffer_type = MYSQL_TYPE_LONG;
		param[3].buffer = &quantita_int;
		param[3].buffer_length = sizeof(quantita_int);
	}

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for product insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while adding the product.");
	}
	else
	{
		printf("Prodotto aggiunto correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void registra_ordine(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[2];

	// Input for the registration routine
	char tavolo[10];
	int tavolo_int;
	int status;
	int ordine_id;
	my_bool is_null;

	// Get the required information
	printf("\nTavolo: ");
	getInput(10, tavolo, false);
	tavolo_int = atoi(tavolo);

	if (!setup_prepared_stmt(&prepared_stmt, "call registraOrdine(?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &tavolo_int;
	param[0].buffer_length = sizeof(tavolo_int);

	param[1].buffer_type = MYSQL_TYPE_LONG;
	param[1].buffer = &ordine_id;
	param[1].buffer_length = sizeof(ordine_id);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred.");
		goto out;
	}

	do
	{
		int i;
		int num_fields;		 /* number of columns in result */
		MYSQL_FIELD *fields; /* for result set metadata */
		MYSQL_BIND *rs_bind; /* for output buffers */

		/* the column count is > 0 if there is a result set */
		/* 0 if the result is only the final status packet */
		num_fields = mysql_stmt_field_count(prepared_stmt);

		if (num_fields > 0)
		{
			/* there is a result set to fetch */
			printf("Number of columns in result: %d\n", (int)num_fields);

			/* what kind of result set is this? */
			if (conn->server_status & SERVER_PS_OUT_PARAMS)
				printf("this result set contains OUT/INOUT parameters\n");
			else
				printf("this result set is produced by the procedure\n");

			MYSQL_RES *rs_metadata = mysql_stmt_result_metadata(prepared_stmt);

			fields = mysql_fetch_fields(rs_metadata);

			rs_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * num_fields);
			if (!rs_bind)
			{
				printf("Cannot allocate output buffers\n");
				exit(1);
			}
			memset(rs_bind, 0, sizeof(MYSQL_BIND) * num_fields);

			/* set up and bind result set output buffers */
			for (i = 0; i < num_fields; ++i)
			{
				rs_bind[i].buffer_type = fields[i].type;
				rs_bind[i].is_null = &is_null;

				switch (fields[i].type)
				{
				case MYSQL_TYPE_LONG:
					rs_bind[i].buffer = (char *)&(ordine_id);
					rs_bind[i].buffer_length = sizeof(ordine_id);
					break;

				default:
					fprintf(stderr, "ERROR: unexpected type: %d.\n", fields[i].type);
					exit(1);
				}
			}

			status = mysql_stmt_bind_result(prepared_stmt, rs_bind);

			/* fetch and display result set rows */
			while (1)
			{
				status = mysql_stmt_fetch(prepared_stmt);

				if (status == 1 || status == MYSQL_NO_DATA)
					break;

				for (i = 0; i < num_fields; ++i)
				{
					switch (rs_bind[i].buffer_type)
					{
					case MYSQL_TYPE_LONG:
						if (*rs_bind[i].is_null)
							printf(" val[%d] = NULL;", i);
						else
							printf("Ordine registrato ID: %lu;",
								   (long)*((int *)rs_bind[i].buffer));
						break;

					default:
						printf("  unexpected type (%d)\n",
							   rs_bind[i].buffer_type);
					}
				}
				printf("\n");
			}

			mysql_free_result(rs_metadata); /* free metadata */
			free(rs_bind);					/* free output buffers */
		}
		else
		{
			/* no columns = final status packet */
			printf("End of procedure output\n");
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(prepared_stmt);
		if (status > 0)
			goto out;
	} while (status == 0);

out:
	mysql_stmt_close(prepared_stmt);
}

static void consegna_ordine(MYSQL *conn)
{
	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[2];

	char ordine[46];
	int ordine_int;
	char *username;

	printf("\nOrdine: ");
	getInput(46, ordine, false);
	ordine_int = atoi(ordine);
	username = conn->user;

	if (!setup_prepared_stmt(&prepared_stmt, "call consegnaOrdine(?,?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &ordine_int;
	param[0].buffer_length = sizeof(ordine_int);

	param[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[1].buffer = username;
	param[1].buffer_length = strlen(username);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters\n", true);
	}

	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred.");
	}
	else
	{
		printf("Ordine consegnato correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void visualizza_tavoli(MYSQL *conn, char *username)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	// Input for the registration routine
	int status;
	int tavolo;
	char stato[46];
	int capienza;
	my_bool is_null;

	printf("username: %s\n", username);
	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaTavoliInGestione(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_STRING;
	param[0].buffer = username;
	param[0].buffer_length = strlen(username);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred.");
		goto out;
	}

	do
	{
		int i;
		int num_fields;		 /* number of columns in result */
		MYSQL_FIELD *fields; /* for result set metadata */
		MYSQL_BIND *rs_bind; /* for output buffers */

		/* the column count is > 0 if there is a result set */
		/* 0 if the result is only the final status packet */
		num_fields = mysql_stmt_field_count(prepared_stmt);

		if (num_fields > 0)
		{
			/* there is a result set to fetch */
			/* what kind of result set is this? */
			if (conn->server_status & SERVER_PS_OUT_PARAMS)
				printf("this result set contains OUT/INOUT parameters\n");
			else
				printf("this result set is produced by the procedure\n");

			MYSQL_RES *rs_metadata = mysql_stmt_result_metadata(prepared_stmt);

			fields = mysql_fetch_fields(rs_metadata);

			rs_bind = (MYSQL_BIND *)malloc(sizeof(MYSQL_BIND) * num_fields);
			if (!rs_bind)
			{
				printf("Cannot allocate output buffers\n");
				exit(1);
			}
			memset(rs_bind, 0, sizeof(MYSQL_BIND) * num_fields);

			/* set up and bind result set output buffers */
			for (i = 0; i < num_fields; ++i)
			{
				rs_bind[i].buffer_type = fields[i].type;
				rs_bind[i].is_null = &is_null;

				switch (fields[i].type)
				{
				case MYSQL_TYPE_LONG:
					if (i == 0)
					{
						rs_bind[i].buffer = (char *)&(tavolo);
						rs_bind[i].buffer_length = sizeof(tavolo);
						break;
					}
					else if (i == 2)
					{
						rs_bind[i].buffer = (char *)&(capienza);
						rs_bind[i].buffer_length = sizeof(capienza);
						break;
					}
					break;

				case MYSQL_TYPE_STRING:
					rs_bind[i].buffer = (char *)&(stato);
					rs_bind[i].buffer_length = sizeof(stato);
					break;

				default:
					fprintf(stderr, "ERROR: unexpected type: %d.\n", fields[i].type);
					exit(1);
				}
			}

			status = mysql_stmt_bind_result(prepared_stmt, rs_bind);

			/* fetch and display result set rows */
			while (1)
			{
				status = mysql_stmt_fetch(prepared_stmt);

				if (status == 1 || status == MYSQL_NO_DATA)
					break;

				for (i = 0; i < num_fields; ++i)
				{
					switch (rs_bind[i].buffer_type)
					{
					case MYSQL_TYPE_LONG:
						if (i == 0)
						{
							if (*rs_bind[i].is_null)
								printf(" val[%d] = NULL;", i);
							else
								printf("Tavolo: %u\t", tavolo);
							break;
						}
						else if (i == 2)
						{

							if (*rs_bind[i].is_null)
								printf(" val[%d] = NULL;", i);
							else
								printf("Capienza: %u\t", capienza);
							break;
						}
						break;

					case MYSQL_TYPE_STRING:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else
							printf("Stato: %s\t\t", stato);
						break;

					default:
						printf("  unexpected type (%d)\n",
							   rs_bind[i].buffer_type);
					}
				}
				printf("\n");
			}

			mysql_free_result(rs_metadata); /* free metadata */
			free(rs_bind);					/* free output buffers */
		}
		else
		{
			/* no columns = final status packet */
			printf("End of procedure output\n");
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(prepared_stmt);
		if (status > 0)
			goto out;
	} while (status == 0);

out:
	mysql_stmt_close(prepared_stmt);
}

//OPPORTUNE MODIFICHE
void run_as_cameriere(MYSQL *conn, char *username)
{
	char options[6] = {'1', '2', '3', '4', '5'};
	char op;
	char *user = username;

	printf("Switching to cameriere role...\n");

	if (!parse_config("users/cameriere.json", &conf))
	{
		fprintf(stderr, "Unable to load student configuration\n");
		exit(EXIT_FAILURE);
	}

	if (mysql_change_user(conn, conf.db_username, conf.db_password, conf.database))
	{
		fprintf(stderr, "mysql_change_user() failed\n%s \n", mysql_error(conn));
		exit(EXIT_FAILURE);
	}

	while (true)
	{
		printf("\033[2J\033[H");
		printf("*** What should I do for you? ***\n\n");
		printf("1) Registra un ordine\n");
		printf("2) Aggiungi prodotto a ordine\n");
		printf("3) Consegna ordine\n");
		printf("4) Visualizza tavoli in gestione\n");
		printf("5) Quit\n");

		op = multiChoice("Select an option", options, 6);

		switch (op)
		{
		case '1':
			registra_ordine(conn);
			break;
		case '2':
			aggiungi_prodotto_ordine(conn);
			break;
		case '3':
			consegna_ordine(conn);
			break;
		case '4':
			visualizza_tavoli(conn, user);
			break;
		case '5':
			return;

		default:
			fprintf(stderr, "Invalid condition at %s:%d\n", __FILE__, __LINE__);
			abort();
		}

		getchar();
	}
}
