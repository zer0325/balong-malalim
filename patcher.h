struct defpatch {
	const char *sig;	/* signature */
	uint32_t sigsize;	/* signature length */
	int32_t poffset;	/* offset to the patch point */
						/* from the end of the signature */
};

uint32_t perasebad(uint8_t *buf, uint32_t size);
uint32_t patch(struct defpatch fp, uint8_t *buf, uint32_t fsize, uint32_t ptype);



