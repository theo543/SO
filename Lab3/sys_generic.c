int
sys_khello(struct proc *p, void *v, register_t *retval)
{
	struct sys_khello_args *uap = v;

	const size_t MAXSIZE = 101;
	char *kmsg = malloc(MAXSIZE, M_TEMP, M_WAITOK | M_ZERO);

	copyinstr(SCARG(uap, msg), kmsg, MAXSIZE - 1, NULL);
	printf("%s", kmsg);

	free(kmsg, M_TEMP, MAXSIZE);

	return 0;
}

int
sys_kcopybuf(struct proc *p, void *v, register_t *retval)
{
	struct sys_kcopybuf_args *uap = v;

	size_t buflen = SCARG(uap, len);
	char *buf = malloc(buflen, M_TEMP, M_WAITOK);

	int err = copyin(SCARG(uap, src), buf, buflen);
	if(err != 0) {
		*retval = err;
		goto cleanup;
	}

	err = copyout(buf, SCARG(uap, dst), buflen);
	if(err != 0) {
		*retval = err;
		goto cleanup;
	}

	cleanup:
	free(buf, M_TEMP, buflen);
	return 0;
}
