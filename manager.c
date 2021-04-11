#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"

struct cameriere_turno
{
	int cameriere;
	int turno;
};

static size_t parse_avgs(MYSQL *conn, MYSQL_STMT *stmt, struct cameriere_turno **ret)
{
	int status;
	size_t row = 0;
	MYSQL_BIND param[2];

	int id_cameriere;
	int id_turno;

	if (mysql_stmt_store_result(stmt))
	{
		fprintf(stderr, " mysql_stmt_execute(), 1 failed\n");
		fprintf(stderr, " %s\n", mysql_stmt_error(stmt));
		exit(0);
	}

	*ret = malloc(mysql_stmt_num_rows(stmt) * sizeof(struct cameriere_turno));

	memset(param, 0, sizeof(param));
	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &id_cameriere;
	param[0].buffer_length = sizeof(id_cameriere);

	param[1].buffer_type = MYSQL_TYPE_LONG;
	param[1].buffer = &id_turno;
	param[1].buffer_length = sizeof(id_turno);

	if (mysql_stmt_bind_result(stmt, param))
	{
		finish_with_stmt_error(conn, stmt, "Unable to bind column parameters\n", true);
	}

	/* assemble course general information */
	while (true)
	{
		status = mysql_stmt_fetch(stmt);

		if (status == 1 || status == MYSQL_NO_DATA)
			break;

		(*ret)[row].cameriere = id_cameriere;
		(*ret)[row].turno = id_turno;

		row++;
	}

	return row;
}

static void visualizza_camerieri(MYSQL *conn)
{

	MYSQL_RES *res_set;
	MYSQL_ROW row;
	unsigned int i;

	if (mysql_query(conn, "select * from lavoratore where tipo = 'Cameriere'") != 0)
	{
		fprintf(stderr, "An error occured during query\n%s", mysql_error(conn));
	}
	else
	{
		res_set = mysql_store_result(conn);
		while ((row = mysql_fetch_row(res_set)) != NULL)
		{
			for (i = 0; i < mysql_num_fields(res_set); i++)
			{
				printf("%s\t", row[i] != NULL ? row[i] : "NULL");
			}
			printf("\n");
		}

		mysql_free_result(res_set);
	}
}

static void visualizza_tavoli(MYSQL *conn)
{

	MYSQL_RES *res_set;
	MYSQL_ROW row;
	unsigned int i;

	if (mysql_query(conn, "select * from tavolo") != 0)
	{
		fprintf(stderr, "An error occured during query\n%s", mysql_error(conn));
	}
	else
	{
		res_set = mysql_store_result(conn);
		while ((row = mysql_fetch_row(res_set)) != NULL)
		{
			for (i = 0; i < mysql_num_fields(res_set); i++)
			{
				printf("%s\t", row[i] != NULL ? row[i] : "NULL");
			}
			printf("\n");
		}

		mysql_free_result(res_set);
	}
}

static void visualizza_turni(MYSQL *conn)
{

	MYSQL_RES *res_set;
	MYSQL_ROW row;
	unsigned int i;

	if (mysql_query(conn, "select * from turno") != 0)
	{
		fprintf(stderr, "An error occured during query\n%s", mysql_error(conn));
	}
	else
	{
		res_set = mysql_store_result(conn);
		while ((row = mysql_fetch_row(res_set)) != NULL)
		{
			for (i = 0; i < mysql_num_fields(res_set); i++)
			{
				printf("%s\t", row[i] != NULL ? row[i] : "NULL");
			}
			printf("\n");
		}

		mysql_free_result(res_set);
	}
}

static void assign_tavoloturno(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[2];

	char turno[46];
	char tavolo[46];
	int turno_int;
	int tavolo_int;

	printf("\nTurno: ");
	getInput(46, turno, false);
	printf("\nTavolo: ");
	getInput(46, tavolo, false);
	tavolo_int = atoi(tavolo);
	turno_int = atoi(turno);

	if (!setup_prepared_stmt(&prepared_stmt, "call assegnaTurnoATavolo(?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &turno_int;
	param[0].buffer_length = sizeof(turno_int);

	param[1].buffer_type = MYSQL_TYPE_LONG;
	param[1].buffer = &tavolo_int;
	param[1].buffer_length = sizeof(tavolo_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred.");
	}
	else
	{
		printf("Turno assegnato al tavolo correttamente\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void aggiungi_tavolo(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	char capienza[46];
	int capienza_int;

	printf("\nCapienza: ");
	getInput(46, capienza, false);
	capienza_int = atoi(capienza);

	if (!setup_prepared_stmt(&prepared_stmt, "insert into tavolo (Capienza) values (?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &capienza_int;
	param[0].buffer_length = sizeof(capienza_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for table insertion\n", true);
	}

	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while adding the table.");
	}
	else
	{
		printf("Tavolo inserito correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void create_user(MYSQL *conn)
{
	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[6];
	char options[6] = {'1', '2', '3', '4'};
	char r;

	// Input for the registration routine
	char nome[46];
	char cognome[46];
	char username[46];
	char password[46];
	char ruolo[46];

	// Get the required information
	printf("\nNome: ");
	getInput(46, nome, false);
	printf("\nCognome: ");
	getInput(46, cognome, false);
	printf("\nUsername: ");
	getInput(46, username, false);
	printf("password: ");
	getInput(46, password, true);
	printf("Assign a possible role:\n");
	printf("\t1) Cameriere\n");
	printf("\t2) Pizzaiolo\n");
	printf("\t3) Manager\n");
	printf("\t4) Barista\n");
	r = multiChoice("Select role", options, 4);

	// Convert role into enum value
	switch (r)
	{
	case '1':
		strcpy(ruolo, "Cameriere");
		break;
	case '2':
		strcpy(ruolo, "Pizzaiolo");
		break;
	case '3':
		strcpy(ruolo, "Manager");
		break;
	case '4':
		strcpy(ruolo, "Barista");
		break;
	default:
		fprintf(stderr, "Invalid condition at %s:%d\n", __FILE__, __LINE__);
		abort();
	}

	// Prepare stored procedure call
	if (!setup_prepared_stmt(&prepared_stmt, "call registraLavoratore(?, ?, ?, ?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[0].buffer = nome;
	param[0].buffer_length = strlen(nome);

	param[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[1].buffer = cognome;
	param[1].buffer_length = strlen(cognome);

	param[2].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[2].buffer = ruolo;
	param[2].buffer_length = strlen(ruolo);

	param[3].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[3].buffer = username;
	param[3].buffer_length = strlen(username);

	param[4].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[4].buffer = password;
	param[4].buffer_length = strlen(password);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for user insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while adding the user.");
	}
	else
	{
		printf("Lavoratore registrato correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void visualizza_lavoro(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];
	bool first = true;
	char header[512];
	struct cameriere_turno *cam_turn;
	size_t turni = 0;
	int status;

	char idCameriere[46];
	int idCameriere_int;

	// Get the required information
	printf("\nCameriere: ");
	getInput(46, idCameriere, false);
	idCameriere_int = atoi(idCameriere);

	//Prepara procedura
	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaLavoroDiCameriere(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &idCameriere_int;
	param[0].buffer_length = sizeof(idCameriere_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for user insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while visualizing.");
		goto out;
	}

	do
	{

		if (conn->server_status & SERVER_PS_OUT_PARAMS)
		{
			goto next;
		}

		if (first)
		{
			printf("Primo result set\n");
			parse_avgs(conn, prepared_stmt, &cam_turn);
			first = false;
		}
		else
		{
			sprintf(header, "\nCameriere %u \t Turno: %u\n", cam_turn[turni].cameriere, cam_turn[turni].turno);
			printf("%s", header);
			dump_result_set(conn, prepared_stmt, header);
			turni++;
		}

	next:
		status = mysql_stmt_next_result(prepared_stmt);
		if (status > 0)
			finish_with_stmt_error(conn, prepared_stmt, "Unexpected condition", true);

	} while (status == 0);

out:
	mysql_stmt_close(prepared_stmt);
}

static void assign_cameriereturno(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[2];

	// Input per assegnazione cameriere-turno
	char idCameriere[46];
	char idTurno[46];
	int idCameriere_int;
	int idTurno_int;

	// Get the required information
	printf("\nCameriere: ");
	getInput(46, idCameriere, false);
	printf("\nTurno: ");
	getInput(46, idTurno, false);
	idCameriere_int = atoi(idCameriere);
	idTurno_int = atoi(idTurno);

	//Prepara procedura
	if (!setup_prepared_stmt(&prepared_stmt, "call associaTurnoACameriere(?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &idCameriere_int;
	param[0].buffer_length = sizeof(idCameriere_int);

	param[1].buffer_type = MYSQL_TYPE_LONG;
	param[1].buffer = &idTurno_int;
	param[1].buffer_length = sizeof(idTurno_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for user insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred during assignment.");
	}
	else
	{
		printf("Turno associato al cameriere correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void stampa_scontrino(MYSQL *conn)
{

	//INSERISCI SCONTRINO

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	char ordine[46];
	int ordine_int;

	printf("\nOrdine: ");
	getInput(46, ordine, false);
	ordine_int = atoi(ordine);

	if (!setup_prepared_stmt(&prepared_stmt, "call stampaScontrino(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &ordine_int;
	param[0].buffer_length = sizeof(ordine_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for table insertion\n", true);
	}

	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while adding the table.");
	}
	else
	{
		printf("Scontrino inserito correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);

	//STAMPA SCONTRINO
	// Input for the registration routine
	int status;
	double totale;
	MYSQL_TIME tempo;
	int idScontrino;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call stampaScontrinoOrdine(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &ordine_int;
	param[0].buffer_length = sizeof(ordine_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters \n", true);
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
				case MYSQL_TYPE_DOUBLE:
					rs_bind[i].buffer = (char *)&(totale);
					rs_bind[i].buffer_length = sizeof(totale);
					break;
				case MYSQL_TYPE_TIMESTAMP:
					rs_bind[i].buffer = (char *)&(tempo);
					rs_bind[i].buffer_length = sizeof(tempo);
					break;
				case MYSQL_TYPE_LONG:
					rs_bind[i].buffer = (char *)&(idScontrino);
					rs_bind[i].buffer_length = sizeof(idScontrino);
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
					case MYSQL_TYPE_DOUBLE:
						if (*rs_bind[i].is_null)
							printf("NULL\t");
						else
							printf("Totale: %f;",
								   (double)*((double *)rs_bind[i].buffer));
						break;

					case MYSQL_TYPE_TIMESTAMP:
						if (*rs_bind[i].is_null)
							printf("NULL\t");
						else
							printf("Tempo: %d-%d-%d  %u-%u-%u;",
								   tempo.year, tempo.month, tempo.day, tempo.hour, tempo.minute, tempo.second);
						break;

					case MYSQL_TYPE_LONG:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else if (i == 0)
							printf("idScontrino: %ld;",
								   (long)*((int *)rs_bind[i].buffer));
						else
							printf("Ordine: %d", ordine_int);
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

static void registra_cliente(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[4];

	// Input for the registration routine
	char nome[46];
	char cognome[46];
	char ncommensali[46];
	int ncommensali_int;
	int tavolo;
	int status;
	my_bool is_null;

	// Get the required information
	printf("\nNome: ");
	getInput(46, nome, false);
	printf("\nCognome: ");
	getInput(46, cognome, false);
	printf("\nNumero di commensali: ");
	getInput(5, ncommensali, false);
	ncommensali_int = atoi(ncommensali);

	if (!setup_prepared_stmt(&prepared_stmt, "call registraClienteAutomaticamente(?, ?, ?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[0].buffer = nome;
	param[0].buffer_length = strlen(nome);

	param[1].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[1].buffer = cognome;
	param[1].buffer_length = strlen(cognome);

	param[2].buffer_type = MYSQL_TYPE_LONG;
	param[2].buffer = &ncommensali_int;
	param[2].buffer_length = sizeof(ncommensali_int);

	param[3].buffer_type = MYSQL_TYPE_LONG;
	param[3].buffer = &tavolo;
	param[3].buffer_length = sizeof(tavolo);

	printf("Nome acquisito: %s\n", nome);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for customer insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while registering customer.");
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
			printf("Data: ");
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
					rs_bind[i].buffer = (char *)&(tavolo);
					rs_bind[i].buffer_length = sizeof(tavolo);
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
							printf("Cliente assegnato al tavolo: %ld;",
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

static void visualizza_disponibilitatavolo(MYSQL *conn)
{
	MYSQL_STMT *prepared_stmt;

	// Input for the registration routine
	int tavoloDisp;
	int turnoTavolo;
	int status;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaDisponibilitaTavoli()", conn))
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
					if (i == 0)
					{
						rs_bind[i].buffer = (char *)&(tavoloDisp);
						rs_bind[i].buffer_length = sizeof(tavoloDisp);
						break;
					}
					else if (i == 1)
					{
						rs_bind[i].buffer = (char *)&(turnoTavolo);
						rs_bind[i].buffer_length = sizeof(turnoTavolo);
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
						if (i == 0)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Tavolo: %u;",
									   tavoloDisp);
							break;
						}
						else
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Turno: %u;",
									   turnoTavolo);
							break;
						}

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

static void visualizza_entrateMensili(MYSQL *conn)
{
	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	// Input for the registration routine
	double entrate;
	int status;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call entrateMensili(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_DOUBLE;
	param[0].buffer = &entrate;
	param[0].buffer_length = sizeof(entrate);

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
				case MYSQL_TYPE_DOUBLE:
					rs_bind[i].buffer = (char *)&(entrate);
					rs_bind[i].buffer_length = sizeof(entrate);
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
					case MYSQL_TYPE_DOUBLE:
						if (*rs_bind[i].is_null)
							printf(" val[%d] = NULL;", i);
						else
							printf("Entrate mensili: %f;",
								   (double)*((double *)rs_bind[i].buffer));
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
			//printf("End of procedure output\n");
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(prepared_stmt);
		if (status > 0)
			goto out;
	} while (status == 0);

out:
	mysql_stmt_close(prepared_stmt);
}

static void visualizza_entrateGiornaliere(MYSQL *conn)
{
	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[1];

	// Input for the registration routine
	double entrate;
	int status;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call entrateGiornaliere(?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_DOUBLE;
	param[0].buffer = &entrate;
	param[0].buffer_length = sizeof(entrate);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for customer insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while registering customer.");
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
			printf("Data: ");
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
				case MYSQL_TYPE_DOUBLE:
					rs_bind[i].buffer = (char *)&(entrate);
					rs_bind[i].buffer_length = sizeof(entrate);
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
					case MYSQL_TYPE_DOUBLE:
						if (*rs_bind[i].is_null)
							printf(" val[%d] = NULL;", i);
						else
							printf("Entrate giornaliere: %f;",
								   (double)*((double *)rs_bind[i].buffer));
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
			//printf("End of procedure output\n");
		}

		/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		status = mysql_stmt_next_result(prepared_stmt);
		if (status > 0)
			goto out;
	} while (status == 0);

out:
	mysql_stmt_close(prepared_stmt);
}

static void visualizza_prodotti(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;

	// Input for the registration routine
	char nome[46];
	double prezzo;
	int status;
	bool dispProdotto;
	char tipo[46];
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaProdotti()", conn))
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
				case MYSQL_TYPE_VAR_STRING:
					rs_bind[i].buffer = (char *)&(nome);
					rs_bind[i].buffer_length = sizeof(nome);
					break;
				case MYSQL_TYPE_STRING:
					rs_bind[i].buffer = (char *)&(tipo);
					rs_bind[i].buffer_length = sizeof(tipo);
					break;

				case MYSQL_TYPE_TINY:
					rs_bind[i].buffer = (char *)&(dispProdotto);
					rs_bind[i].buffer_length = sizeof(dispProdotto);
					break;
				case MYSQL_TYPE_DOUBLE:
					rs_bind[i].buffer = (char *)&(prezzo);
					rs_bind[i].buffer_length = sizeof(prezzo);
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
					case MYSQL_TYPE_DOUBLE:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else
							printf("Prezzo %f",
								   prezzo);
						break;

					case MYSQL_TYPE_VAR_STRING:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else
						{
							printf("%s\t", nome);
						}
						break;

					case MYSQL_TYPE_STRING:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else
						{
							printf("%s\t", tipo);
						}
						break;

					case MYSQL_TYPE_TINY:
						if (*rs_bind[i].is_null)
							printf("NULL");
						else
							printf("Disponibilità: %u;",
								   dispProdotto);
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

static void modifica_dispprodotto(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[2];

	// Input per assegnazione cameriere-turno
	char prodotto[46];
	char disp[4];
	int disp_int;
	bool disp_bool;

	// Get the required information
	printf("\nProdotto: ");
	getInput(46, prodotto, false);
	printf("\nDisponibilità: ");
	getInput(4, disp, false);
	disp_int = atoi(disp);
	disp_bool = (bool)disp_int;

	//Prepara procedura
	if (!setup_prepared_stmt(&prepared_stmt, "call modificaDispProdotto(?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_VAR_STRING;
	param[0].buffer = prodotto;
	param[0].buffer_length = strlen(prodotto);

	param[1].buffer_type = MYSQL_TYPE_TINY;
	param[1].buffer = &disp_bool;
	param[1].buffer_length = sizeof(disp_bool);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred during assignment.");
	}
	else
	{
		printf("Disponibilità modificata correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

static void visualizza_gestione(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;

	// Input for the registration routine
	int cameriere;
	int tavolo;
	int turnodilavoro;

	int status;
	my_bool is_null;

	if (!setup_prepared_stmt(&prepared_stmt, "call visualizzaGestione()", conn))
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
					if (i == 0)
					{
						rs_bind[i].buffer = (char *)&(cameriere);
						rs_bind[i].buffer_length = sizeof(cameriere);
						break;
					}
					else if (i == 1)
					{
						rs_bind[i].buffer = (char *)&(tavolo);
						rs_bind[i].buffer_length = sizeof(tavolo);
						break;
					}
					else if (i == 2)
					{
						rs_bind[i].buffer = (char *)&(turnodilavoro);
						rs_bind[i].buffer_length = sizeof(turnodilavoro);
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
						if (i == 0)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Cameriere: %u\t",
									   cameriere);
							break;
						}
						else if (i == 1)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Tavolo: %u\t",
									   tavolo);
							break;
						}
						else if (i == 2)
						{
							if (*rs_bind[i].is_null)
								printf("NULL");
							else
								printf("Turno: %u",
									   turnodilavoro);
							break;
						}
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

static void assign_gestionetavolo(MYSQL *conn)
{

	MYSQL_STMT *prepared_stmt;
	MYSQL_BIND param[3];

	// Input for the registration routine
	char cameriere[46];
	char tavolo[46];
	char turno[46];
	int cameriere_int;
	int tavolo_int;
	int turno_int;

	// Get the required information
	printf("\nCameriere: ");
	getInput(46, cameriere, false);
	printf("\nTavolo: ");
	getInput(46, tavolo, false);
	printf("\nTurno: ");
	getInput(46, turno, false);
	cameriere_int = atoi(cameriere);
	tavolo_int = atoi(tavolo);
	turno_int = atoi(turno);

	// Prepare stored procedure call
	if (!setup_prepared_stmt(&prepared_stmt, "call assegnaCameriereTavoloTurno(?, ?, ?)", conn))
	{
		finish_with_stmt_error(conn, prepared_stmt, "Unable to initialize user insertion statement\n", false);
	}

	// Prepare parameters
	memset(param, 0, sizeof(param));

	param[0].buffer_type = MYSQL_TYPE_LONG;
	param[0].buffer = &cameriere_int;
	param[0].buffer_length = sizeof(cameriere_int);

	param[1].buffer_type = MYSQL_TYPE_LONG;
	param[1].buffer = &tavolo_int;
	param[1].buffer_length = sizeof(tavolo_int);

	param[2].buffer_type = MYSQL_TYPE_LONG;
	param[2].buffer = &turno_int;
	param[2].buffer_length = sizeof(turno_int);

	if (mysql_stmt_bind_param(prepared_stmt, param) != 0)
	{
		finish_with_stmt_error(conn, prepared_stmt, "Could not bind parameters for user insertion\n", true);
	}

	// Run procedure
	if (mysql_stmt_execute(prepared_stmt) != 0)
	{
		print_stmt_error(prepared_stmt, "An error occurred while adding the user.");
	}
	else
	{
		printf("Assegnamento effettuato correttamente...\n");
	}

	mysql_stmt_close(prepared_stmt);
}

//OPPORTUNE MODIFICHE
void run_as_manager(MYSQL *conn)
{
	char options[19] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'};
	char op;

	printf("Switching to manager role...\n");

	if (!parse_config("users/manager.json", &conf))
	{
		fprintf(stderr, "Unable to load manager configuration\n");
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
		printf("Scegli un opzione possibile\n\n");
		printf("1) Assegna un turno ad un tavolo\n");
		printf("2) Assegna un turno ad un cameriere\n");
		printf("3) Registra un nuovo lavoratore\n");
		printf("4) Assegna un tavolo ad un cameriere in un certo turno\n");
		printf("5) Visualizza le assegnazioni cameriere-tavolo\n");
		printf("6) Visualizza i turni dei camerieri\n");
		printf("7) Visualizza i turni dei tavoli\n");
		printf("8) Visualizza i turni\n");
		printf("9) Aggiungi un tavolo\n");
		printf("a) Stampa lo scontrino per un ordine\n");
		printf("b) Visualizza tavoli\n");
		printf("c) Visualizza camerieri\n");
		printf("d) Registra cliente\n");
		printf("e) Entrate giornaliere\n");
		printf("f) Entrate mensili\n");
		printf("g) Visualizza prodotti\n");
		printf("h) Modifica disponibilità prodotti\n");
		printf("i) Quit\n");

		op = multiChoice("Select an option", options, 19);

		switch (op)
		{
		case '1':
			assign_tavoloturno(conn);
			break;
		case '2':
			assign_cameriereturno(conn);
			break;
		case '3':
			create_user(conn);
			break;
		case '4':
			assign_gestionetavolo(conn);
			break;
		case '5':
			visualizza_gestione(conn);
			break;
		case '6':
			visualizza_lavoro(conn);
			break;
		case '7':
			visualizza_disponibilitatavolo(conn);
			break;
		case '8':
			visualizza_turni(conn);
			break;
		case '9':
			aggiungi_tavolo(conn);
			break;
		case 'a':
			stampa_scontrino(conn);
			break;
		case 'b':
			visualizza_tavoli(conn);
			break;
		case 'c':
			visualizza_camerieri(conn);
			break;
		case 'd':
			registra_cliente(conn);
			break;
		case 'e':
			visualizza_entrateGiornaliere(conn);
			break;
		case 'f':
			visualizza_entrateMensili(conn);
			break;
		case 'g':
			visualizza_prodotti(conn);
			break;
		case 'h':
			modifica_dispprodotto(conn);
			break;
		case 'i':
			return;

		default:
			fprintf(stderr, "Invalid condition at %s:%d\n", __FILE__, __LINE__);
			abort();
		}

		getchar();
	}
}
