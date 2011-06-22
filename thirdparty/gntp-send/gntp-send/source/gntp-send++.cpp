#include <growl++.hpp>

int main(int argc, char **argv)
{
	const char *n[2] = { "alice" , "bob" };
	Growl *growl = new Growl(GROWL_TCP,NULL,"gntp_send++",(const char **const)n,2);
	growl->Notify("bob","title","message");

	delete(growl);

	return 0;
}
