import pathlib

import pkg_resources

README = 'README.md'
TEST_PARSER = 'tests/python/test_parser.py'
PUBLIC_MODULE_TIME = 'modules/public/module_time.json'
PRIVATE_MODULE_PHISH = 'modules/private/module_phish.json'


def main():
    print('Hello, world')

    # Print which files exist
    print(pkg_resources.resource_exists(__name__, README))
    print(pkg_resources.resource_exists(__name__, TEST_PARSER))
    print(pkg_resources.resource_exists(__name__, PUBLIC_MODULE_TIME))
    print(pkg_resources.resource_exists(__name__, PRIVATE_MODULE_PHISH))

    # Assert that module files exist
    assert pkg_resources.resource_exists(__name__, PUBLIC_MODULE_TIME)

    # Assert that the files exist

    # Check file
    print('=' * 20)
    file_path = pathlib.Path(
        pkg_resources.resource_filename(__name__, PUBLIC_MODULE_TIME))
    print(file_path)
    assert file_path.exists()
    contents = pkg_resources.resource_string(__name__, PUBLIC_MODULE_TIME)
    assert contents

    print('=' * 20)

    # assert pkg_resources.resource_exists(__name__, PRIVATE_MODULE_PHISH)

    # # We can convert to string from bytes optionally
    # print(contents.decode('utf-8')[0:300])
    # print('=' * 20)

    # # Exactly the same check, just to make sure
    # print('=' * 20)
    # file_path = pathlib.Path(
    #     pkg_resources.resource_filename(__name__, PRIVATE_MODULE_PHISH))
    # print(file_path)
    # assert file_path.exists()
    # contents = pkg_resources.resource_string(__name__, PRIVATE_MODULE_PHISH)
    # assert contents

    # print('=' * 20)
    # print(contents[0:300])
    # print('=' * 20)

    # import pdb
    # pdb.set_trace()
