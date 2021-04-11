#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

static void bevanda_pronta(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	char componente[46];
	int componente_int;

	printf("\nComponente: ");
	getInput(46, componente, false);
	componente_int = atoi(componente);

	if (!setup_prepared_stmt(&prepared_stmt, "call bevandapronta(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &componente_int;
	param[0].buffer_length = sizeof(componente_int);

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
		printf("Bevanda segnata come pronta correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void bevande_da_preparare(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;

	char nome[46];
	int ordine_rif;
	int status;
	int id_componente;
	int quantita;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaBevandeDaPreparare()", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize statement\n", false);
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
					if (i == 1)
					{
						rs_bind[i].buffer = (char *)&(ordine_rif);
						rs_bind[i].buffer_length = sizeof(ordine_rif);
						break;
					}
					else if (i == 2)
					{

						rs_bind[i].buffer = (char *)&(quantita);
						rs_bind[i].buffer_length = sizeof(quantita);
						break;
					}
					else if (i == 3)
					{

						rs_bind[i].buffer = (char *)&(id_componente);
						rs_bind[i].buffer_length = sizeof(id_componente);
						break;
					}
					break;

				case MYSQL_TYPE_VAR_STRING:
					if (i == 0)
					{
						rs_bind[i].buffer = (char *)&(nome);
						rs_bind[i].buffer_length = sizeof(nome);
						break;
					}
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
						if (i == 1)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Ordine %lu\t",
									   (long)*((int *)rs_bind[i].buffer));
							break;
						}
						else if (i == 2)
						{

							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Quantità %lu\t",
									   (long)*((int *)rs_bind[i].buffer));
							break;
						}
						else if (i == 3)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("IDComponente %lu\t",
									   (long)*((int *)rs_bind[i].buffer));
							break;
						}
						break;

					case MYSQL_TYPE_VAR_STRING:
						if (i == 0)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
							{
								printf("Nome: %s\t", (char *)rs_bind[i].buffer);
							}
						}
						else if (i == 2)
						{
							if (*rs_bind[i].is_null)
								printf("Aggiunta: NULL\t");
							else
							{
								printf("Aggiunta %s\t", (char *)rs_bind[i].buffer);
							}
						}
						break;

					default:
						//printf("  unexpected type (%d)\n",
						//rs_bind[i].buffer_type);
						break;
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
void run_as_barista(MYSQL *conn)
{
	char options[3] = {'1', '2', '3'};
	char op;

	printf("Switching to barista role...\n");

	if (!parse_config("users/barista.json", &conf))
	{
		fprintf(stderr, "Unable to load barista configuration\n");
		exit(EXIT_FAILURE);
	}

	if (mysql_change_user(conn, conf.db_username, conf.db_password, conf.database))
	{
		fprintf(stderr, "mysql_change_user() failed\n");
		exit(EXIT_FAILURE);
	}

	while (true)
	{
		printf("\033[2J\033[H");
		printf("1) Visualizza bevande da preparare\n");
		printf("2) Segna bevanda come pronta\n");
		printf("3) Quit\n");

		op = multiChoice("Select an option", options, 3);

		switch (op)
		{
		case '1':
			bevande_da_preparare(conn);
			break;
		case '2':
			bevanda_pronta(conn);
			break;
		case '3':
			return;

		default:
			fprintf(stderr, "Invalid condition at %s:%d\n", __FILE__, __LINE__);
			abort();
		}

		getchar();
	}
}
