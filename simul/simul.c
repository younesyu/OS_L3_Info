
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**********************************************************
** Codage d'une instruction (32 bits)
***********************************************************/

typedef struct {
	unsigned OP: 10;  /* code operation (10 bits)  */
	unsigned i:   3;  /* nu 1er registre (3 bits)  */
	unsigned j:   3;  /* nu 2eme registre (3 bits) */
	short    ARG;     /* argument (16 bits)        */
} INST;


/**********************************************************
** definition de la memoire simulee
***********************************************************/

typedef int WORD;  /* un mot est un entier 32 bits  */

WORD mem[128];     /* memoire                       */


/**********************************************************
** Codes associes aux instructions
***********************************************************/

#define INST_ADD	(0)
#define INST_SUB	(1)
#define INST_CMP	(2)
#define INST_IFGT	(3)
#define INST_NOP	(4)
#define INST_JUMP	(5)
#define INST_HALT	(6)
#define INST_SYSC	(7)


/**********************************************************
** Placer une instruction en memoire
***********************************************************/

void make_inst(int adr, unsigned code, unsigned i, unsigned j, short arg) {
	union { WORD word; INST fields; } inst;
	inst.fields.OP  = code;
	inst.fields.i   = i;
	inst.fields.j   = j;
	inst.fields.ARG = arg;
	mem[adr] = inst.word;
}


/**********************************************************
** Codes associes aux interruptions
***********************************************************/

#define INT_INIT	(1)
#define INT_SEGV	(2)
#define INT_INST	(3)
#define INT_CLOCK	(4)
#define INT_EXIT	(5)
#define INT_PUTI	(6)


/**********************************************************
** Le Mot d'Etat du Processeur (PSW)
***********************************************************/

typedef struct PSW {    /* Processor Status Word */
	WORD PC;        /* Program Counter */
	WORD SB;        /* Segment Base */
	WORD SS;        /* Segment Size */
	WORD IN;        /* Interrupt number */
	WORD DR[8];     /* Data Registers */
	WORD AC;        /* Accumulateur */
	INST RI;        /* Registre instruction */
} PSW;


/**********************************************************
** Simulation de la CPU (mode utilisateur)
***********************************************************/

/* instruction d'addition */
PSW cpu_ADD(PSW m) {
	m.AC = m.DR[m.RI.i] += (m.DR[m.RI.j] + m.RI.ARG);
	m.PC += 1;
	return m;
}

/* instruction de soustraction */
PSW cpu_SUB(PSW m) {
	m.AC = m.DR[m.RI.i] -= (m.DR[m.RI.j] + m.RI.ARG);
	m.PC += 1;
	return m;
}

/* instruction de comparaison */
PSW cpu_CMP(PSW m) {
	m.AC = (m.DR[m.RI.i] - (m.DR[m.RI.j] + m.RI.ARG));
	m.PC += 1;
	return m;
}

/* instruction de condition if greater then */
PSW cpu_IFGT(PSW m) {
	if (m.AC > 0) m.PC = m.DR[m.RI.i] + m.RI.ARG;
	else m.PC += 1;
	return m;
}

/* instruction de non opération */
PSW cpu_NOP(PSW m) {
	m.PC += 1;
	printf("No operation.\n");
	return m;
}

/* instruction de saut */
PSW cpu_JUMP(PSW m) {
	printf("\t\t\t\t\tJump!\n");
	m.PC = m.DR[m.RI.i] + m.RI.ARG;
	return m;
}

/* instruction de halte */
PSW cpu_HALT(PSW m) {
	printf("HALT\n");
	exit(EXIT_SUCCESS);
	return m;
}


/**********************************************************
** Simulation de la CPU (mode système)
***********************************************************/

PSW cpu_SYSC(PSW m) {
	m.PC += 1;
	
	switch(m.RI.ARG) {
		
		case SYSC_EXIT:
			printf("Syscall exit.\n");
			m.IN = INT_EXIT;
			break;
		
		case SYSC_PUTI:
			m.IN = INT_PUTI;
			break;
	}

	return m;
}



/* Simulation de la CPU */
PSW cpu(PSW m) {
	union { WORD word; INST in; } inst;
	static int instruction_counter = 0;
	
	/*** lecture et decodage de l'instruction ***/
	if (m.PC < 0 || m.PC >= m.SS) {
		m.IN = INT_SEGV;
		return (m);
	}
	inst.word = mem[m.PC + m.SB];
	m.RI = inst.in;
	/*** execution de l'instruction ***/
	switch (m.RI.OP) {
	case INST_ADD:
		m = cpu_ADD(m);
		break;
	case INST_SUB:
		m = cpu_SUB(m);
		break;
	case INST_CMP:
		m = cpu_CMP(m);
		break;
	case INST_IFGT:
		m = cpu_IFGT(m);
		break;
	case INST_NOP:
		m = cpu_NOP(m);
		break;
	case INST_JUMP:
		m = cpu_JUMP(m);
		break;
	case INST_HALT:
		m = cpu_HALT(m);
		break;
	case INST_SYSC:
		return cpu_SYSC(m);

	default:
		/*** interruption instruction inconnue ***/
		m.IN = INT_INST;
		return (m);
	}

	/*** interruption clock apres chaque 3 instructions ***/
	instruction_counter++;
	if (instruction_counter % 3 == 0) {
		instruction_counter = 0;
		m.IN = INT_CLOCK;
	}
	else m.IN = 0;
	return m;
}


/**********************************************************
** Demarrage du systeme
***********************************************************/

PSW systeme_init(void) {
	PSW cpu;

	printf("Booting.\n");
	/*** creation d'un programme ***/
	make_inst(0, INST_SUB, 1, 1, 0); /* R1 = 0 */
	make_inst(1, INST_SUB, 2, 2, -1000);   /* R2 = 1000 */
	make_inst(2, INST_SUB, 3, 3, -10);   /* R3 = 10 */
	make_inst(3, INST_SUB, 4, 4, 0);   /* R4 = 0 */
	make_inst(4, INST_CMP, 1, 2, 0); /* AC = (R1 - R2) */
	make_inst(5, INST_IFGT, 4, -1, 11); /* if (AC > 0) PC = R4 + 11 */
	make_inst(6, INST_NOP, -1, -1, -1);
	make_inst(7, INST_NOP, -1, -1, -1);
	make_inst(8, INST_NOP, -1, -1, -1);
	make_inst(9, INST_ADD, 1, 3, 0); /* R1 += R3 */
	make_inst(10, INST_JUMP, 4, -1, 4); /* PC = R4 + 4 */
	make_inst(11, INST_HALT, -1, -1, -1);


	
	/*** valeur initiale du PSW ***/
	memset (&cpu, 0, sizeof(cpu));
	cpu.PC = 0;
	cpu.SB = 0;
	cpu.SS = 20;

	return cpu;
}


/**********************************************************
** Simulation du systeme (mode systeme)
***********************************************************/

PSW systeme(PSW m) {
	if(m.IN) printf("--- Interruption n°%d ---\n", m.IN);

	switch (m.IN) {
		case INT_INIT:
			return (systeme_init());
		case INT_SEGV:
			printf("Segmentation fault.\n");
			exit(EXIT_FAILURE);
		case INT_CLOCK:
			printf("\t\t\t\t\tProgram Counter = %d\n", m.PC);
			for (int i = 0; i < 8; ++i)
				printf("Data Register %d : %d\n", i, m.DR[i]);
			break;
		case INT_INST:
			printf("Unknown instruction.\n");
			exit(EXIT_FAILURE);
		case INT_PUTI:
			printf("Data Register %d : %d\n", m.RI.i, m.DR[m.RI.i]);
			break;
		case INT_EXIT:
			exit(EXIT_SUCCESS);
	}
	return m;
}


/**********************************************************
** fonction principale
** (ce code ne doit pas etre modifie !)
***********************************************************/

int main(void) {
	PSW mep;
	
	mep.IN = INT_INIT; /* interruption INIT */	
	while (1) {
		mep = systeme(mep);
		mep = cpu(mep);
	}
	
	return (EXIT_SUCCESS);
}

